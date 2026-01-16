/**
 * bonus_handler.c - Bonus Round (Tiebreaker)
 * 
 * PURPOSE:
 * Handles tie-breaking when multiple players have the same score at critical positions.
 * Uses a card-drawing mechanism to randomly select one player.
 * 
 * STATE USAGE:
 * - Score: MatchPlayerState.score - READ via get_match_player()
 * - Eliminated: MatchPlayerState.eliminated - READ/UPDATE
 * - Connected: UserSession.state - READ via session_get_by_account()
 * 
 * LOCAL TRACKING (Bonus-specific):
 * - participants[]: Players who are tied and must draw
 * - spectators[]: Players who watch (not tied or already eliminated)
 * - card_stack[]: Shuffled cards to draw from
 * - drawn_cards[]: Cards each participant drew
 * 
 * FLOW:
 * 1. After round scoring, check_and_trigger_bonus() is called
 * 2. If tie detected, initialize bonus round
 * 3. Participants draw cards one by one
 * 4. After all draw, reveal results with 2s delay
 * 5. Apply result (eliminate/declare winner)
 * 6. Transition to next phase
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "handlers/bonus_handler.h"
#include "handlers/session_manager.h"
#include "handlers/match_manager.h"
#include "handlers/start_game_handler.h"
#include "handlers/end_game_handler.h"
#include "transport/room_manager.h"
#include "db/core/db_client.h"
#include "db/repo/match_repo.h"         // For db_match_event_insert, db_match_question_insert
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include <cjson/cJSON.h>

//==============================================================================
// CONSTANTS
//==============================================================================
#define MAX_BONUS_PARTICIPANTS 8
#define REVEAL_DELAY_MS 2000
#define RESULT_DISPLAY_MS 3000
#define TRANSITION_DELAY_MS 5000

//==============================================================================
// DEBUG: Force trigger bonus for testing
// Set to 1 to always trigger bonus after any round (for testing)
// Set to 0 for normal production behavior
//==============================================================================
#define DEBUG_FORCE_BONUS 0

//==============================================================================
// BONUS ROUND CONTEXT
//==============================================================================

typedef struct {
    int32_t account_id;
    PlayerBonusState state;
    CardType drawn_card;
    time_t drawn_at;
} BonusParticipant;

typedef struct {
    uint32_t match_id;
    int after_round;           // Round that triggered bonus (1, 2, or 3)
    BonusType type;            // ELIMINATION or WINNER_SELECTION
    BonusState state;
    
    // Participants (tied players who must draw)
    BonusParticipant participants[MAX_BONUS_PARTICIPANTS];
    int participant_count;
    
    // Spectators (watching players)
    int32_t spectators[MAX_BONUS_PARTICIPANTS];
    int spectator_count;
    
    // Card stack
    CardType card_stack[MAX_BONUS_PARTICIPANTS];
    int cards_remaining;
    int total_cards;
    
    // Tracking
    int drawn_count;
    int32_t eliminated_player_id;  // Result: who got eliminated
    int32_t winner_player_id;      // Result: who won (for WINNER_SELECTION)
    
    time_t started_at;
    time_t reveal_at;
    
    // Timer
    bool timer_running;
    pthread_t timer_thread;
} BonusContext;

static BonusContext g_bonus = {0};
static pthread_mutex_t g_bonus_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations
static MatchState* get_match(void);
static MatchPlayerState* get_match_player(int32_t account_id);
static bool is_connected(int32_t account_id);
static int get_socket(int32_t account_id);
static BonusParticipant* find_participant(int32_t account_id);
static void send_json(int fd, MessageHeader *req, uint16_t cmd, const char *json);
static void broadcast_to_all(MessageHeader *req, uint16_t cmd, const char *json);
static void broadcast_to_participants(MessageHeader *req, uint16_t cmd, const char *json);
static void shuffle_cards(void);
static void initialize_bonus(uint32_t match_id, int after_round, BonusType type,
                            int32_t *tied_players, int tied_count);
static void notify_participants(MessageHeader *req);
static void notify_spectators(MessageHeader *req);
static void process_card_draw(int32_t account_id, MessageHeader *req);
static void check_all_drawn(MessageHeader *req);
static void reveal_results(MessageHeader *req);
static void apply_results(MessageHeader *req);
static void transition_to_next_phase(MessageHeader *req);
static void reset_context(void);
static void force_finish_draw(MessageHeader *req);
static void start_bonus_timer(void);
static void stop_bonus_timer(void);

//==============================================================================
// HELPER: Get states from other PICs (READ ONLY)
//==============================================================================
static MatchState* get_match(void) {
    if (g_bonus.match_id == 0) return NULL;
    return match_get_by_id(g_bonus.match_id);
}

static MatchPlayerState* get_match_player(int32_t account_id) {
    MatchState *match = get_match();
    if (!match) return NULL;
    for (int i = 0; i < match->player_count; i++) {
        if (match->players[i].account_id == account_id) {
            return &match->players[i];
        }
    }
    return NULL;
}

static bool is_connected(int32_t account_id) {
    UserSession *session = session_get_by_account(account_id);
    if (!session) return false;
    return session->state != SESSION_PLAYING_DISCONNECTED;
}

static int get_socket(int32_t account_id) {
    UserSession *session = session_get_by_account(account_id);
    if (session && session->state != SESSION_PLAYING_DISCONNECTED) {
        return session->socket_fd;
    }
    return -1;
}

//==============================================================================
// HELPER: Local tracking
//==============================================================================
static BonusParticipant* find_participant(int32_t account_id) {
    for (int i = 0; i < g_bonus.participant_count; i++) {
        if (g_bonus.participants[i].account_id == account_id) {
            return &g_bonus.participants[i];
        }
    }
    return NULL;
}

static bool is_spectator(int32_t account_id) {
    for (int i = 0; i < g_bonus.spectator_count; i++) {
        if (g_bonus.spectators[i] == account_id) {
            return true;
        }
    }
    return false;
}

//==============================================================================
// HELPER: Response/Broadcast
//==============================================================================
static void send_json(int fd, MessageHeader *req, uint16_t cmd, const char *json) {
    if (fd <= 0 || !json) return;
    forward_response(fd, req, cmd, json, (uint32_t)strlen(json));
}

static void broadcast_to_all(MessageHeader *req, uint16_t cmd, const char *json) {
    if (!json) return;
    
    // Send to participants
    for (int i = 0; i < g_bonus.participant_count; i++) {
        int fd = get_socket(g_bonus.participants[i].account_id);
        if (fd > 0) {
            forward_response(fd, req, cmd, json, (uint32_t)strlen(json));
        }
    }
    
    // Send to spectators
    for (int i = 0; i < g_bonus.spectator_count; i++) {
        int fd = get_socket(g_bonus.spectators[i]);
        if (fd > 0) {
            forward_response(fd, req, cmd, json, (uint32_t)strlen(json));
        }
    }
}

static void broadcast_to_participants(MessageHeader *req, uint16_t cmd, const char *json) {
    if (!json) return;
    for (int i = 0; i < g_bonus.participant_count; i++) {
        int fd = get_socket(g_bonus.participants[i].account_id);
        if (fd > 0) {
            forward_response(fd, req, cmd, json, (uint32_t)strlen(json));
        }
    }
}

//==============================================================================
// HELPER: Shuffle cards (Fisher-Yates)
//==============================================================================
static void shuffle_cards(void) {
    for (int i = g_bonus.total_cards - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        CardType tmp = g_bonus.card_stack[i];
        g_bonus.card_stack[i] = g_bonus.card_stack[j];
        g_bonus.card_stack[j] = tmp;
    }
}

//==============================================================================
// HELPER: Reset context
//==============================================================================
static void reset_context(void) {
    pthread_mutex_lock(&g_bonus_mutex);
    memset(&g_bonus, 0, sizeof(g_bonus));
    pthread_mutex_unlock(&g_bonus_mutex);
    printf("[Bonus] Context reset\n");
}

//==============================================================================
// TIMER LOGIC & AUTO DRAW
//==============================================================================
static void* bonus_timer_thread(void* arg) {
    (void)arg;
    sleep(20); // 20s timeout for selecting card
    
    pthread_mutex_lock(&g_bonus_mutex);
    if (!g_bonus.timer_running || g_bonus.state != BONUS_STATE_DRAWING) {
        pthread_mutex_unlock(&g_bonus_mutex);
        return NULL;
    }
    g_bonus.timer_running = false;
    pthread_mutex_unlock(&g_bonus_mutex);
    
    printf("[Bonus] Timeout reached - forcing finish draw\n");
    MessageHeader dummy = {0};
    dummy.magic = htons(MAGIC_NUMBER);
    dummy.version = PROTOCOL_VERSION;
    force_finish_draw(&dummy);
    return NULL;
}

static void start_bonus_timer(void) {
    pthread_mutex_lock(&g_bonus_mutex);
    if (g_bonus.timer_running) {
        pthread_mutex_unlock(&g_bonus_mutex);
        return;
    }
    g_bonus.timer_running = true;
    pthread_mutex_unlock(&g_bonus_mutex);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&g_bonus.timer_thread, &attr, bonus_timer_thread, NULL) != 0) {
        printf("[Bonus] Failed to start timer thread\n");
    }
    pthread_attr_destroy(&attr);
}

static void stop_bonus_timer(void) {
    pthread_mutex_lock(&g_bonus_mutex);
    g_bonus.timer_running = false;
    pthread_mutex_unlock(&g_bonus_mutex);
}

static void force_finish_draw(MessageHeader *req) {
    bool any_drawn = false;
    
    pthread_mutex_lock(&g_bonus_mutex);
    for (int i=0; i < g_bonus.participant_count; i++) {
        if (g_bonus.participants[i].state == PLAYER_BONUS_WAITING_TO_DRAW) {
             if (g_bonus.cards_remaining > 0) {
                 CardType drawn = g_bonus.card_stack[g_bonus.total_cards - g_bonus.cards_remaining];
                 g_bonus.cards_remaining--;
                 
                 g_bonus.participants[i].drawn_card = drawn;
                 g_bonus.participants[i].state = PLAYER_BONUS_CARD_DRAWN;
                 g_bonus.participants[i].drawn_at = time(NULL);
                 g_bonus.drawn_count++;
                 any_drawn = true;
                 
                 printf("[Bonus] Auto-assigned card %d to player %d\n", drawn, g_bonus.participants[i].account_id);
             }
        }
    }
    pthread_mutex_unlock(&g_bonus_mutex);
    
    if (any_drawn) {
        cJSON *broadcast = cJSON_CreateObject();
        cJSON_AddBoolToObject(broadcast, "timeout", true);
        cJSON_AddStringToObject(broadcast, "message", "Timeout reached - cards auto-assigned");
        char *json = cJSON_PrintUnformatted(broadcast);
        broadcast_to_all(req, OP_S2C_BONUS_PLAYER_DREW, json); 
        free(json);
        cJSON_Delete(broadcast);
        
        check_all_drawn(req);
    }


}

//==============================================================================
// INITIALIZE BONUS ROUND
//==============================================================================
static void initialize_bonus(uint32_t match_id, int after_round, BonusType type,
                            int32_t *tied_players, int tied_count) {
    pthread_mutex_lock(&g_bonus_mutex);
    
    // Reset and setup
    memset(&g_bonus, 0, sizeof(g_bonus));
    g_bonus.match_id = match_id;
    g_bonus.after_round = after_round;
    g_bonus.type = type;
    g_bonus.state = BONUS_STATE_INITIALIZING;
    g_bonus.started_at = time(NULL);
    
    // Setup participants (tied players)
    g_bonus.participant_count = tied_count;
    for (int i = 0; i < tied_count; i++) {
        g_bonus.participants[i].account_id = tied_players[i];
        g_bonus.participants[i].state = PLAYER_BONUS_WAITING_TO_DRAW;
        g_bonus.participants[i].drawn_card = CARD_TYPE_SAFE;
        g_bonus.participants[i].drawn_at = 0;
    }
    
    // Setup spectators (other players in match)
    MatchState *match = match_get_by_id(match_id);
    if (match) {
        for (int i = 0; i < match->player_count; i++) {
            int32_t acc_id = match->players[i].account_id;
            bool is_participant = false;
            for (int j = 0; j < tied_count; j++) {
                if (tied_players[j] == acc_id) {
                    is_participant = true;
                    break;
                }
            }
            if (!is_participant) {
                g_bonus.spectators[g_bonus.spectator_count++] = acc_id;
            }
        }
    }
    
    // Create card stack: 1 ELIMINATED, rest SAFE
    g_bonus.total_cards = tied_count;
    g_bonus.cards_remaining = tied_count;
    if (g_bonus.type == BONUS_TYPE_ELIMINATION) {
        g_bonus.card_stack[0] = CARD_TYPE_ELIMINATED;
    } else {
        g_bonus.card_stack[0] = CARD_TYPE_WINNER;
    }

    for (int i = 1; i < tied_count; i++) {
        g_bonus.card_stack[i] = CARD_TYPE_SAFE;
    }
    
    // Shuffle cards
    shuffle_cards();
    
    g_bonus.state = BONUS_STATE_DRAWING;
    
    // CRITICAL: Unlock mutex BEFORE starting timer to prevent deadlock
    // Timer callback may need to acquire this mutex
    pthread_mutex_unlock(&g_bonus_mutex);
    
    // Start timer (after unlocking to prevent deadlock)
    start_bonus_timer();
    
    printf("[Bonus] Initialized: match=%u after_round=%d type=%s participants=%d\n",
           match_id, after_round,
           type == BONUS_TYPE_ELIMINATION ? "ELIMINATION" : "WINNER_SELECTION",
           tied_count);
}

//==============================================================================
// NOTIFY PARTICIPANTS
//==============================================================================
static void notify_participants(MessageHeader *req) {
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "role", "participant");
    cJSON_AddStringToObject(obj, "bonus_type", 
        g_bonus.type == BONUS_TYPE_ELIMINATION ? "elimination" : "winner_selection");
    cJSON_AddNumberToObject(obj, "after_round", g_bonus.after_round);
    
    const char *reason = g_bonus.type == BONUS_TYPE_ELIMINATION 
        ? "Tie at lowest score - one player will be eliminated"
        : "Tie at highest score - winner will be decided";
    cJSON_AddStringToObject(obj, "reason", reason);
    
    // List other participants
    cJSON *others = cJSON_CreateArray();
    for (int i = 0; i < g_bonus.participant_count; i++) {
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "account_id", g_bonus.participants[i].account_id);
        
        char name[32];
        snprintf(name, sizeof(name), "Player%d", g_bonus.participants[i].account_id);
        cJSON_AddStringToObject(p, "name", name);
        cJSON_AddItemToArray(others, p);
    }
    cJSON_AddItemToObject(obj, "participants", others);
    
    cJSON_AddNumberToObject(obj, "total_cards", g_bonus.total_cards);
    cJSON_AddStringToObject(obj, "instruction", "Click DRAW to draw a card from the stack");
    
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    
    if (json) {
        broadcast_to_participants(req, OP_S2C_BONUS_PARTICIPANT, json);
        free(json);
    }
}

//==============================================================================
// NOTIFY SPECTATORS
//==============================================================================
static void notify_spectators(MessageHeader *req) {
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "role", "spectator");
    cJSON_AddStringToObject(obj, "bonus_type",
        g_bonus.type == BONUS_TYPE_ELIMINATION ? "elimination" : "winner_selection");
    cJSON_AddNumberToObject(obj, "after_round", g_bonus.after_round);
    
    // Build clear message about what's happening
    char message[256];
    if (g_bonus.type == BONUS_TYPE_ELIMINATION) {
        snprintf(message, sizeof(message), 
            "%d players tied at lowest score. Bonus round in progress - one will be eliminated!",
            g_bonus.participant_count);
    } else {
        snprintf(message, sizeof(message),
            "%d players tied at highest score. Bonus round in progress - winner will be decided!",
            g_bonus.participant_count);
    }
    cJSON_AddStringToObject(obj, "message", message);
    cJSON_AddNumberToObject(obj, "participant_count", g_bonus.participant_count);
    
    // List participants
    cJSON *participants = cJSON_CreateArray();
    for (int i = 0; i < g_bonus.participant_count; i++) {
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "account_id", g_bonus.participants[i].account_id);
        
        char name[32];
        snprintf(name, sizeof(name), "Player%d", g_bonus.participants[i].account_id);
        cJSON_AddStringToObject(p, "name", name);
        cJSON_AddItemToArray(participants, p);
    }
    cJSON_AddItemToObject(obj, "participants", participants);
    
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    
    if (json) {
        for (int i = 0; i < g_bonus.spectator_count; i++) {
            int fd = get_socket(g_bonus.spectators[i]);
            if (fd > 0) {
                forward_response(fd, req, OP_S2C_BONUS_SPECTATOR, json, (uint32_t)strlen(json));
            }
        }
        free(json);
    }
}

//==============================================================================
// CHECK AND TRIGGER BONUS (Called by round handlers)
//==============================================================================
bool check_and_trigger_bonus(uint32_t match_id, int after_round) {
    MatchState *match = match_get_by_id(match_id);
    if (!match) {
        printf("[Bonus] Match %u not found\n", match_id);
        return false;
    }
    
    printf("[Bonus] Checking for ties: match=%u after_round=%d mode=%d\n",
           match_id, after_round, match->mode);
    
    // Collect active (non-eliminated) players and their scores
    typedef struct {
        int32_t account_id;
        int32_t score;
    } ScoreEntry;
    
    ScoreEntry entries[MAX_MATCH_PLAYERS];
    int count = 0;
    
    for (int i = 0; i < match->player_count; i++) {
        MatchPlayerState *mp = &match->players[i];
        if (!mp->eliminated && !mp->forfeited) {
            entries[count].account_id = mp->account_id;
            entries[count].score = mp->score;
            count++;
        }
    }
    
    if (count < 2) {
        printf("[Bonus] Not enough active players (%d) for bonus check\n", count);
        return false;
    }
    
#if DEBUG_FORCE_BONUS
    //==========================================================================
    // DEBUG MODE: Force trigger bonus with all active players
    //==========================================================================
    printf("[Bonus] DEBUG_FORCE_BONUS enabled - forcing bonus round!\n");
    
    int32_t tied_players[MAX_MATCH_PLAYERS];
    int tied_count = count;
    for (int i = 0; i < count; i++) {
        tied_players[i] = entries[i].account_id;
    }
    
    BonusType bonus_type = (after_round == 3) ? BONUS_TYPE_WINNER_SELECTION : BONUS_TYPE_ELIMINATION;
    
    printf("[Bonus] DEBUG: Triggering bonus with %d players, type=%s\n", 
           tied_count, bonus_type == BONUS_TYPE_ELIMINATION ? "ELIMINATION" : "WINNER_SELECTION");
    
    // Initialize bonus context
    initialize_bonus(match_id, after_round, bonus_type, tied_players, tied_count);
    
    // Create a dummy header for notifications
    MessageHeader dummy_hdr = {0};
    dummy_hdr.magic = htons(MAGIC_NUMBER);
    dummy_hdr.version = PROTOCOL_VERSION;
    
    // Notify all players
    notify_participants(&dummy_hdr);
    notify_spectators(&dummy_hdr);
    
    return true;
#endif
    
    // Determine what score we're looking for based on context
    int target_score = 0;
    BonusType bonus_type_prod;
    
    if (after_round == 3) {
        // Round 3 finished - find highest score for winner selection
        // (both ELIMINATION and SCORING modes)
        target_score = entries[0].score;
        for (int i = 1; i < count; i++) {
            if (entries[i].score > target_score) {
                target_score = entries[i].score;
            }
        }
        bonus_type_prod = BONUS_TYPE_WINNER_SELECTION;
        printf("[Bonus] Round 3 - looking for highest score: %d\n", target_score);
    } else {
        // Round 1 or 2 - only check in ELIMINATION mode
        if (match->mode != MODE_ELIMINATION) {
            printf("[Bonus] Scoring mode - no elimination after round %d\n", after_round);
            return false;
        }
        
        // Find lowest score for elimination
        target_score = entries[0].score;
        for (int i = 1; i < count; i++) {
            if (entries[i].score < target_score) {
                target_score = entries[i].score;
            }
        }
        bonus_type_prod = BONUS_TYPE_ELIMINATION;
        printf("[Bonus] Round %d - looking for lowest score: %d\n", after_round, target_score);
    }
    
    // Count how many players have the target score
    int32_t tied_players_prod[MAX_MATCH_PLAYERS];
    int tied_count_prod = 0;
    
    for (int i = 0; i < count; i++) {
        if (entries[i].score == target_score) {
            tied_players_prod[tied_count_prod++] = entries[i].account_id;
        }
    }
    
    printf("[Bonus] Found %d players with target score %d\n", tied_count_prod, target_score);
    
    if (tied_count_prod < 2) {
        // No tie - no bonus needed
        printf("[Bonus] No tie detected - bonus not needed\n");
        return false;
    }
    
    // Trigger bonus round!
    printf("[Bonus] TRIGGERING BONUS ROUND for %d tied players\n", tied_count_prod);
    
    // Initialize bonus context
    printf("[Bonus] About to call initialize_bonus...\n");
    initialize_bonus(match_id, after_round, bonus_type_prod, tied_players_prod, tied_count_prod);
    printf("[Bonus] initialize_bonus() returned\n");
    
    // Create a dummy header for notifications
    MessageHeader dummy_hdr_prod = {0};
    dummy_hdr_prod.magic = htons(MAGIC_NUMBER);
    dummy_hdr_prod.version = PROTOCOL_VERSION;
    
    // Notify all players
    printf("[Bonus] About to notify participants...\n");
    notify_participants(&dummy_hdr_prod);
    printf("[Bonus] About to notify spectators...\n");
    notify_spectators(&dummy_hdr_prod);
    
    // Save to database
    if (match->db_match_id > 0) {
        cJSON *bonus_payload = cJSON_CreateObject();
        cJSON_AddNumberToObject(bonus_payload, "match_id", match->db_match_id);
        cJSON_AddNumberToObject(bonus_payload, "round_no", 4);  // Bonus is round 4
        cJSON_AddStringToObject(bonus_payload, "round_type", "BONUS");
        cJSON_AddNumberToObject(bonus_payload, "question_idx", 0);
        
        cJSON *question_data = cJSON_CreateObject();
        cJSON_AddStringToObject(question_data, "type", 
            bonus_type_prod == BONUS_TYPE_ELIMINATION ? "elimination" : "winner_selection");
        cJSON_AddNumberToObject(question_data, "after_round", after_round);
        cJSON_AddNumberToObject(question_data, "participant_count", tied_count_prod);
        cJSON_AddItemToObject(bonus_payload, "question", question_data);
        
        cJSON *db_result = NULL;
        db_error_t db_err = db_post("match_question", bonus_payload, &db_result);
        if (db_err == DB_OK) {
            printf("[Bonus] Saved bonus round to DB\n");
        }
        cJSON_Delete(bonus_payload);
        if (db_result) cJSON_Delete(db_result);
    }
    
    printf("[Bonus] check_and_trigger_bonus() RETURNING true\n");
    return true;
}

//==============================================================================
// PROCESS CARD DRAW
//==============================================================================
static void process_card_draw(int32_t account_id, MessageHeader *req) {
    BonusParticipant *p = find_participant(account_id);
    if (!p) {
        printf("[Bonus] Player %d not a participant\n", account_id);
        return;
    }
    
    if (p->state != PLAYER_BONUS_WAITING_TO_DRAW) {
        printf("[Bonus] Player %d already drew\n", account_id);
        return;
    }
    
    if (g_bonus.cards_remaining <= 0) {
        printf("[Bonus] No cards remaining!\n");
        return;
    }
    
    pthread_mutex_lock(&g_bonus_mutex);
    
    // Draw card from top of stack
    CardType drawn = g_bonus.card_stack[g_bonus.total_cards - g_bonus.cards_remaining];
    g_bonus.cards_remaining--;
    
    p->drawn_card = drawn;
    p->state = PLAYER_BONUS_CARD_DRAWN;
    p->drawn_at = time(NULL);
    g_bonus.drawn_count++;
    
    pthread_mutex_unlock(&g_bonus_mutex);
    
    printf("[Bonus] Player %d drew card: %s (remaining: %d)\n",
           account_id, drawn == CARD_TYPE_ELIMINATED ? "ELIMINATED" : "SAFE",
           g_bonus.cards_remaining);
    
    // Send confirmation to drawer (don't reveal card type yet!)
    int fd = get_socket(account_id);
    if (fd > 0) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddTrueToObject(obj, "success");
        cJSON_AddStringToObject(obj, "message", "Card drawn successfully");
        cJSON_AddNumberToObject(obj, "drawn_at", (int)p->drawn_at);
        
        char *json = cJSON_PrintUnformatted(obj);
        cJSON_Delete(obj);
        
        if (json) {
            send_json(fd, req, OP_S2C_BONUS_CARD_DRAWN, json);
            free(json);
        }
    }
    
    // Broadcast to all that someone drew
    cJSON *broadcast = cJSON_CreateObject();
    cJSON_AddNumberToObject(broadcast, "player_id", account_id);
    
    char name[32];
    snprintf(name, sizeof(name), "Player%d", account_id);
    cJSON_AddStringToObject(broadcast, "player_name", name);
    cJSON_AddNumberToObject(broadcast, "drawn_at", (int)p->drawn_at);
    cJSON_AddNumberToObject(broadcast, "cards_remaining", g_bonus.cards_remaining);
    
    // List who hasn't drawn yet
    cJSON *remaining = cJSON_CreateArray();
    for (int i = 0; i < g_bonus.participant_count; i++) {
        if (g_bonus.participants[i].state == PLAYER_BONUS_WAITING_TO_DRAW) {
            cJSON *r = cJSON_CreateObject();
            cJSON_AddNumberToObject(r, "account_id", g_bonus.participants[i].account_id);
            cJSON_AddItemToArray(remaining, r);
        }
    }
    cJSON_AddItemToObject(broadcast, "remaining_players", remaining);
    
    char *bcast_json = cJSON_PrintUnformatted(broadcast);
    cJSON_Delete(broadcast);
    
    if (bcast_json) {
        broadcast_to_all(req, OP_S2C_BONUS_PLAYER_DREW, bcast_json);
        free(bcast_json);
    }
    
    // Save to database
    MatchState *match = get_match();
    MatchPlayerState *mp = get_match_player(account_id);
    if (match && mp && match->db_match_id > 0 && mp->match_player_id > 0) {
        cJSON *ans_payload = cJSON_CreateObject();
        // We need to find the bonus question ID... for simplicity, skip for now
        // In production, you'd query or store the question_id from initialize_bonus
        cJSON_Delete(ans_payload);
    }
    
    // Check if all have drawn
    check_all_drawn(req);
}

//==============================================================================
// CHECK IF ALL DRAWN
//==============================================================================
static void check_all_drawn(MessageHeader *req) {
    if (g_bonus.drawn_count >= g_bonus.participant_count) {
        printf("[Bonus] All participants have drawn - preparing reveal\n");
        
        stop_bonus_timer();
        
        g_bonus.state = BONUS_STATE_REVEALING;
        g_bonus.reveal_at = time(NULL) + (REVEAL_DELAY_MS / 1000);
        
        // Wait 2 seconds then reveal
        usleep(REVEAL_DELAY_MS * 1000);
        
        reveal_results(req);
    }
}

//==============================================================================
// REVEAL RESULTS
//==============================================================================
static void reveal_results(MessageHeader *req) {
    printf("[Bonus] Revealing results...\n");
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddTrueToObject(obj, "success");
    cJSON_AddStringToObject(obj, "bonus_type",
        g_bonus.type == BONUS_TYPE_ELIMINATION ? "elimination" : "winner_selection");
    
    // Build results array
    cJSON *results = cJSON_CreateArray();
    int32_t eliminated_id = 0;
    
    for (int i = 0; i < g_bonus.participant_count; i++) {
        BonusParticipant *p = &g_bonus.participants[i];
        
        cJSON *r = cJSON_CreateObject();
        cJSON_AddNumberToObject(r, "player_id", p->account_id);
        
        char name[32];
        snprintf(name, sizeof(name), "Player%d", p->account_id);
        cJSON_AddStringToObject(r, "name", name);
        
        if (p->drawn_card == CARD_TYPE_ELIMINATED) {
             cJSON_AddStringToObject(r, "card", "eliminated");
             p->state = PLAYER_BONUS_REVEALED_ELIMINATED;
             eliminated_id = p->account_id;
        } else if (p->drawn_card == CARD_TYPE_WINNER) {
             cJSON_AddStringToObject(r, "card", "winner");
             p->state = PLAYER_BONUS_REVEALED_WINNER;
        } else {
             cJSON_AddStringToObject(r, "card", "safe");
             p->state = PLAYER_BONUS_REVEALED_SAFE;
        }
        
        cJSON_AddItemToArray(results, r);
    }
    
    cJSON_AddItemToObject(obj, "results", results);
    
    // Add eliminated player info
    if (eliminated_id > 0) {
        cJSON *elim_info = cJSON_CreateObject();
        cJSON_AddNumberToObject(elim_info, "id", eliminated_id);
        char elim_name[32];
        snprintf(elim_name, sizeof(elim_name), "Player%d", eliminated_id);
        cJSON_AddStringToObject(elim_info, "name", elim_name);
        cJSON_AddItemToObject(obj, "eliminated_player", elim_info);
        g_bonus.eliminated_player_id = eliminated_id;
    }
    
    // Determine winner for WINNER_SELECTION type
    if (g_bonus.type == BONUS_TYPE_WINNER_SELECTION) {
        for (int i = 0; i < g_bonus.participant_count; i++) {
            if (g_bonus.participants[i].drawn_card == CARD_TYPE_WINNER) {
                g_bonus.winner_player_id = g_bonus.participants[i].account_id;
                break;
            }
        }
        
        if (g_bonus.winner_player_id > 0) {
            cJSON *winner_info = cJSON_CreateObject();
            cJSON_AddNumberToObject(winner_info, "id", g_bonus.winner_player_id);
            char winner_name[32];
            snprintf(winner_name, sizeof(winner_name), "Player%d", g_bonus.winner_player_id);
            cJSON_AddStringToObject(winner_info, "name", winner_name);
            cJSON_AddItemToObject(obj, "winner", winner_info);
        }
    }
    
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    
    if (json) {
        broadcast_to_all(req, OP_S2C_BONUS_REVEAL, json);
        free(json);
    }
    
    // Wait for display, then apply results
    usleep(RESULT_DISPLAY_MS * 1000);
    
    apply_results(req);
}


//==============================================================================
// APPLY RESULTS
//==============================================================================
static void apply_results(MessageHeader *req) {
    printf("[Bonus] Applying results...\n");
    
    g_bonus.state = BONUS_STATE_APPLYING;
    
    MatchState *match = get_match();
    if (!match) return;
    
    if (g_bonus.type == BONUS_TYPE_ELIMINATION) {
        // Eliminate the player who drew ELIMINATED card
        MatchPlayerState *mp = get_match_player(g_bonus.eliminated_player_id);
        if (mp) {
            mp->eliminated = 1;
            mp->eliminated_at_round = g_bonus.after_round;
            
            printf("[Bonus] Player %d ELIMINATED via bonus round\n", 
                   g_bonus.eliminated_player_id);
            
            // Update database
            if (mp->match_player_id > 0) {
                cJSON *update = cJSON_CreateObject();
                cJSON_AddBoolToObject(update, "eliminated", true);
                
                char filter[64];
                snprintf(filter, sizeof(filter), "id=eq.%d", mp->match_player_id);
                
                cJSON *result = NULL;
                db_patch("match_players", filter, update, &result);
                cJSON_Delete(update);
                if (result) cJSON_Delete(result);
            }
            
            // Save elimination event
            // Note: player_id in match_events refers to accounts.id, not match_players.id
            if (match->db_match_id > 0) {
                db_error_t event_err = db_match_event_insert(
                    match->db_match_id,
                    mp->account_id,  // Use account_id, not match_player_id
                    "BONUS_ELIMINATED",
                    4,  // Bonus round
                    0
                );
                if (event_err == DB_SUCCESS) {
                    printf("[Bonus] Saved elimination event to DB\n");
                } else {
                    printf("[Bonus] Failed to save elimination event: err=%d\n", event_err);
                }
            }
            
            // Send elimination notification to the player
            int fd = get_socket(g_bonus.eliminated_player_id);
            if (fd > 0) {
                uint32_t room_id = room_find_by_player_fd(fd);
                cJSON *ntf = cJSON_CreateObject();
                cJSON_AddNumberToObject(ntf, "player_id", g_bonus.eliminated_player_id);
                cJSON_AddNumberToObject(ntf, "room_id", room_id);
                cJSON_AddStringToObject(ntf, "reason", "BONUS_ROUND");
                cJSON_AddNumberToObject(ntf, "round", g_bonus.after_round);
                cJSON_AddStringToObject(ntf, "message", "You were eliminated in the bonus round!");
                
                char *ntf_json = cJSON_PrintUnformatted(ntf);
                cJSON_Delete(ntf);
                
                if (ntf_json) {
                    send_json(fd, req, NTF_ELIMINATION, ntf_json);
                    free(ntf_json);
                }
                
                // Change session state to LOBBY (Staying in room)
                UserSession *elim_session = session_get_by_account(g_bonus.eliminated_player_id);
                if (elim_session) {
                    session_mark_lobby(elim_session);
                    printf("[Bonus] Changed player %d session state to LOBBY (Staying in room)\n", g_bonus.eliminated_player_id);
                    
                    room_id = room_find_by_player_fd(fd);
                    if (room_id > 0) {
                        room_set_ready(room_id, fd, false);
                        broadcast_player_list(room_id);
                        printf("[Bonus] Reset player %d readiness in room %u\n", g_bonus.eliminated_player_id, room_id);
                    }
                }
            }
        }
    } else {
        // WINNER_SELECTION - mark the winner
        MatchPlayerState *winner = get_match_player(g_bonus.winner_player_id);
        if (winner) {
            // Mark all other tied players as not winner
            for (int i = 0; i < g_bonus.participant_count; i++) {
                MatchPlayerState *mp = get_match_player(g_bonus.participants[i].account_id);
                if (mp && mp->match_player_id > 0) {
                    bool is_winner = (mp->account_id == g_bonus.winner_player_id);
                    
                    cJSON *update = cJSON_CreateObject();
                    cJSON_AddBoolToObject(update, "winner", is_winner);
                    
                    char filter[64];
                    snprintf(filter, sizeof(filter), "id=eq.%d", mp->match_player_id);
                    
                    cJSON *result = NULL;
                    db_patch("match_players", filter, update, &result);
                    cJSON_Delete(update);
                    if (result) cJSON_Delete(result);
                }
            }
            
            printf("[Bonus] Player %d declared WINNER via bonus round\n",
                   g_bonus.winner_player_id);
        }
    }
    
    g_bonus.state = BONUS_STATE_COMPLETED;
    
    // Transition to next phase
    transition_to_next_phase(req);
}

//==============================================================================
// TRANSITION TO NEXT PHASE
//==============================================================================
static void transition_to_next_phase(MessageHeader *req) {
    printf("[Bonus] Transitioning to next phase...\n");
    
    MatchState *match = get_match();
    if (!match) return;
    
    int after_round = g_bonus.after_round;
    BonusType bonus_type = g_bonus.type;
    int32_t eliminated_id = g_bonus.eliminated_player_id;
    int32_t winner_id = g_bonus.winner_player_id;
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddTrueToObject(obj, "success");
    
    if (bonus_type == BONUS_TYPE_ELIMINATION) {
        // After elimination bonus, continue to next round
        int next_round = after_round + 1;
        
        if (next_round <= 3) {
            // ⭐ IMPORTANT: Advance match state to next round
            if (match->current_round_idx < match->round_count - 1) {
                match->current_round_idx++;
                printf("[Bonus] Advanced match to round %d (idx=%d)\n", 
                       next_round, match->current_round_idx);
                
                // Initialize next round state
                RoundState *next_round_state = &match->rounds[match->current_round_idx];
                next_round_state->status = ROUND_PENDING;
                next_round_state->current_question_idx = 0;
            }
            
            cJSON_AddStringToObject(obj, "next_phase", "NEXT_ROUND");
            cJSON_AddNumberToObject(obj, "next_round", next_round);
            cJSON_AddNumberToObject(obj, "countdown", TRANSITION_DELAY_MS / 1000);
            
            // Build scoreboard with players array (matching ALL_FINISHED format)
            cJSON *players = cJSON_CreateArray();
            for (int i = 0; i < match->player_count; i++) {
                MatchPlayerState *mp = &match->players[i];
                cJSON *p = cJSON_CreateObject();
                cJSON_AddNumberToObject(p, "id", mp->account_id);
                cJSON_AddNumberToObject(p, "score", mp->score);
                cJSON_AddBoolToObject(p, "eliminated", mp->eliminated != 0);
                cJSON_AddBoolToObject(p, "connected", true);
                
                char name[32];
                snprintf(name, sizeof(name), "Player%d", mp->account_id);
                cJSON_AddStringToObject(p, "name", name);
                
                // Rank based on score (higher = better)
                cJSON_AddNumberToObject(p, "rank", i + 1);
                
                cJSON_AddItemToArray(players, p);
            }
            cJSON_AddItemToObject(obj, "players", players);
            cJSON_AddItemToObject(obj, "scoreboard", cJSON_Duplicate(players, 1));
            
            // Add round info for frontend compatibility
            cJSON_AddNumberToObject(obj, "round", after_round);
            cJSON_AddNumberToObject(obj, "current_round", after_round);
            cJSON_AddBoolToObject(obj, "can_continue", true);
            
            char elim_msg[128];
            snprintf(elim_msg, sizeof(elim_msg), "Player%d eliminated in Bonus Round", eliminated_id);
            cJSON_AddStringToObject(obj, "message", elim_msg);
            cJSON_AddStringToObject(obj, "status_message", elim_msg);
        } else {
            // Bonus elimination after Round 3
            // Check if only 1 player remains -> end game
            int active_count = 0;
            for (int i = 0; i < match->player_count; i++) {
                if (!match->players[i].eliminated && !match->players[i].forfeited) {
                    active_count++;
                }
            }
            
            if (active_count == 1) {
                // Only 1 player left - they win!
                cJSON_AddStringToObject(obj, "next_phase", "MATCH_ENDED");
                printf("[Bonus] After elimination, only 1 player remains - ending game\n");
            } else if (active_count > 1) {
                // Still multiple players - this shouldn't normally happen after Round 3 elimination
                // but handle it gracefully by ending the game anyway
                cJSON_AddStringToObject(obj, "next_phase", "MATCH_ENDED");
                printf("[Bonus] After Round 3 elimination, %d players remain - ending game\n", active_count);
            } else {
                // No active players?!
                cJSON_AddStringToObject(obj, "next_phase", "MATCH_ENDED");
                printf("[Bonus] ERROR: No active players after Round 3 elimination!\n");
            }
        }
    } else {
        // WINNER_SELECTION - match ends
        cJSON_AddStringToObject(obj, "next_phase", "MATCH_ENDED");
        
        // Build final scores
        cJSON *final_scores = cJSON_CreateArray();
        for (int i = 0; i < match->player_count; i++) {
            MatchPlayerState *mp = &match->players[i];
            cJSON *p = cJSON_CreateObject();
            cJSON_AddNumberToObject(p, "id", mp->account_id);
            cJSON_AddNumberToObject(p, "player_id", mp->account_id);
            cJSON_AddNumberToObject(p, "score", mp->score);
            cJSON_AddBoolToObject(p, "winner", mp->account_id == winner_id);
            cJSON_AddBoolToObject(p, "eliminated", mp->eliminated != 0);
            
            char name[32];
            snprintf(name, sizeof(name), "Player%d", mp->account_id);
            cJSON_AddStringToObject(p, "name", name);
            
            cJSON_AddItemToArray(final_scores, p);
        }
        cJSON_AddItemToObject(obj, "final_scores", final_scores);
        cJSON_AddItemToObject(obj, "players", cJSON_Duplicate(final_scores, 1));
        
        // Winner info
        cJSON *winner = cJSON_CreateObject();
        cJSON_AddNumberToObject(winner, "id", winner_id);
        char winner_name[32];
        snprintf(winner_name, sizeof(winner_name), "Player%d", winner_id);
        cJSON_AddStringToObject(winner, "name", winner_name);
        
        MatchPlayerState *winner_mp = get_match_player(winner_id);
        if (winner_mp) {
            cJSON_AddNumberToObject(winner, "final_score", winner_mp->score);
        }
        cJSON_AddItemToObject(obj, "winner", winner);
        
        cJSON_AddTrueToObject(obj, "bonus_winner");
        cJSON_AddStringToObject(obj, "message", "Winner decided by Bonus Round!");
    }
    
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    
    if (json) {
        printf("[Bonus] Sending transition: %s\n", json);
        broadcast_to_all(req, OP_S2C_BONUS_TRANSITION, json);
        free(json);
    }
    
    // Store winner_id before reset for end game trigger
    int32_t bonus_winner = winner_id;
    uint32_t match_id_copy = g_bonus.match_id;
    int after_round_copy = g_bonus.after_round;
    BonusType type_copy = g_bonus.type;
    
    // Reset bonus context
    reset_context();
    
    // Trigger end game if match should end
    if (type_copy == BONUS_TYPE_WINNER_SELECTION && bonus_winner > 0) {
        // Winner selection after Round 3 - end game with bonus winner
        printf("[Bonus] Triggering end game with bonus winner: %d\n", bonus_winner);
        trigger_end_game(match_id_copy, bonus_winner);
    } else if (type_copy == BONUS_TYPE_ELIMINATION && after_round_copy == 3) {
        // Elimination bonus after Round 3 - match should end
        printf("[Bonus] Elimination bonus after Round 3 complete - triggering end game\n");
        trigger_end_game(match_id_copy, -1);
    }
}

//==============================================================================
// HANDLER: Draw Card (OP_C2S_BONUS_DRAW_CARD)
//==============================================================================
static void handle_draw_card(int fd, MessageHeader *req, const char *payload) {
    if (req->length < 4) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid payload\"}");
        return;
    }
    
    uint32_t match_id;
    memcpy(&match_id, payload, 4);
    match_id = ntohl(match_id);
    
    if (match_id != g_bonus.match_id) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid match\"}");
        return;
    }
    
    if (g_bonus.state != BONUS_STATE_DRAWING) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Not in drawing phase\"}");
        return;
    }
    
    UserSession *session = session_get_by_socket(fd);
    if (!session) {
        send_json(fd, req, ERR_NOT_LOGGED_IN, "{\"success\":false,\"error\":\"Not logged in\"}");
        return;
    }
    
    BonusParticipant *p = find_participant(session->account_id);
    if (!p) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Not a participant\"}");
        return;
    }
    
    if (p->state != PLAYER_BONUS_WAITING_TO_DRAW) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Already drew card\"}");
        return;
    }
    
    process_card_draw(session->account_id, req);
}

//==============================================================================
// HANDLER: Ready (OP_C2S_BONUS_READY)
//==============================================================================
static void handle_bonus_ready(int fd, MessageHeader *req, const char *payload) {
    if (req->length < 4) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid payload\"}");
        return;
    }
    
    uint32_t match_id;
    memcpy(&match_id, payload, 4);
    match_id = ntohl(match_id);
    
    UserSession *session = session_get_by_socket(fd);
    if (!session) {
        send_json(fd, req, ERR_NOT_LOGGED_IN, "{\"success\":false,\"error\":\"Not logged in\"}");
        return;
    }
    
    // Check if this player is in bonus
    if (match_id != g_bonus.match_id || g_bonus.state == BONUS_STATE_NONE) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"No active bonus\"}");
        return;
    }
    
    // Send current state
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddTrueToObject(obj, "success");
    cJSON_AddNumberToObject(obj, "match_id", g_bonus.match_id);
    cJSON_AddNumberToObject(obj, "state", g_bonus.state);
    cJSON_AddStringToObject(obj, "bonus_type",
        g_bonus.type == BONUS_TYPE_ELIMINATION ? "elimination" : "winner_selection");
    cJSON_AddNumberToObject(obj, "after_round", g_bonus.after_round);
    cJSON_AddNumberToObject(obj, "participant_count", g_bonus.participant_count);
    cJSON_AddNumberToObject(obj, "drawn_count", g_bonus.drawn_count);
    cJSON_AddNumberToObject(obj, "cards_remaining", g_bonus.cards_remaining);
    
    BonusParticipant *p = find_participant(session->account_id);
    if (p) {
        cJSON_AddStringToObject(obj, "role", "participant");
        cJSON_AddNumberToObject(obj, "player_state", p->state);
    } else if (is_spectator(session->account_id)) {
        cJSON_AddStringToObject(obj, "role", "spectator");
    }
    
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    
    if (json) {
        send_json(fd, req, OP_S2C_BONUS_INIT, json);
        free(json);
    }
}

//==============================================================================
// DISCONNECT HANDLER
//==============================================================================
void handle_bonus_disconnect(int client_fd) {
    if (g_bonus.state == BONUS_STATE_NONE) return;
    
    UserSession *session = session_get_by_socket(client_fd);
    if (!session) return;
    
    int32_t account_id = session->account_id;
    BonusParticipant *p = find_participant(account_id);
    
    if (!p) return;
    
    printf("[Bonus] Player %d disconnected during bonus\n", account_id);
    
    // If player hasn't drawn yet, auto-draw for them
    if (p->state == PLAYER_BONUS_WAITING_TO_DRAW && g_bonus.state == BONUS_STATE_DRAWING) {
        printf("[Bonus] Auto-drawing for disconnected player %d\n", account_id);
        process_card_draw(account_id, NULL);
    }
}

//==============================================================================
// API: Check if bonus is active
//==============================================================================
bool is_bonus_active(uint32_t match_id) {
    return (g_bonus.match_id == match_id && 
            g_bonus.state != BONUS_STATE_NONE &&
            g_bonus.state != BONUS_STATE_COMPLETED);
}

//==============================================================================
// API: Get bonus state
//==============================================================================
BonusState get_bonus_state(uint32_t match_id) {
    if (g_bonus.match_id != match_id) return BONUS_STATE_NONE;
    return g_bonus.state;
}

//==============================================================================
// MAIN DISPATCHER
//==============================================================================
void handle_bonus(int client_fd, MessageHeader *req_header, const char *payload) {
    printf("[Bonus] cmd=0x%04X len=%u\n", req_header->command, req_header->length);
    
    switch (req_header->command) {
        case OP_C2S_BONUS_READY:
            handle_bonus_ready(client_fd, req_header, payload);
            break;
        case OP_C2S_BONUS_DRAW_CARD:
            handle_draw_card(client_fd, req_header, payload);
            break;
        default:
            printf("[Bonus] Unknown cmd: 0x%04X\n", req_header->command);
            break;
    }
}
