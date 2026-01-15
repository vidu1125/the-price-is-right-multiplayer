/**
 * round3_handler.c - Round 3: Bonus Wheel
 * Players spin a wheel (5-100) up to 2 times, with special scoring.
 * 
 * STATE USAGE:
 * - Score: MatchPlayerState.score - READ/UPDATE via get_match_player()
 * - Eliminated: MatchPlayerState.eliminated - READ
 * - Connected: UserSession.state - READ via session_get_by_account()
 * 
 * LOCAL TRACKING (Round-specific):
 * - first_spin: First spin result (0 = not spun yet)
 * - second_spin: Second spin result (0 = not spun yet)
 * - spin_count: Number of spins (0, 1, or 2)
 * - decision_pending: Waiting for player decision after first spin
 * - ready: Is player ready to start the round?
 * 
 * Flow:
 * 1. All players ready → Round starts
 * 2. Server prompts first spin
 * 3. Player spins → Server returns result (5-100)
 * 4. Player decides: Continue (spin again) or Stop
 * 5. If Continue → Second spin → Calculate final bonus
 * 6. If Stop → Calculate final bonus from first spin only
 * 7. Apply bonus to score, check elimination, show final results
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <stdint.h>

#include "handlers/session_manager.h"
#include "handlers/match_manager.h"
#include "handlers/start_game_handler.h"
#include "db/core/db_client.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include <cjson/cJSON.h>

//==============================================================================
// CONSTANTS
//==============================================================================
#define SPIN_MIN_VALUE        5
#define SPIN_MAX_VALUE        100
#define MAX_SPINS             2
#define DECISION_TIMEOUT_MS   10000  // 10 seconds to decide

//==============================================================================
// ROUND 3 EXECUTION CONTEXT
//==============================================================================

typedef struct {
    int32_t account_id;
    int     first_spin;      // First spin result (0 = not spun)
    int     second_spin;     // Second spin result (0 = not spun)
    int     spin_count;      // Number of spins (0, 1, or 2)
    bool    decision_pending; // Waiting for continue/stop decision
    bool    finished;        // Player finished (calculated bonus)
    bool    ready;           // Ready for round to start
} R3_PlayerState;

typedef struct {
    uint32_t match_id;
    int      round_index;
    bool     is_active;
    
    R3_PlayerState players[MAX_MATCH_PLAYERS];
    int      player_count;
    int      ready_count;
    int      finished_count;
} R3_Context;

static R3_Context g_r3 = {0};
static pthread_mutex_t g_r3_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations
static MatchState* get_match(void);
static RoundState* get_round(void);
static MatchPlayerState* get_match_player(int32_t account_id);
static bool is_connected(int32_t account_id);
static int get_socket(int32_t account_id);
static R3_PlayerState* find_player(int32_t account_id);
static R3_PlayerState* add_player(int32_t account_id);
static void send_json(int fd, MessageHeader *req, uint16_t cmd, const char *json);
static void broadcast_json(MessageHeader *req, uint16_t cmd, const char *json);
static int generate_spin_result(void);
static int calculate_bonus(int first_spin, int second_spin);
static void process_player_finished(int32_t account_id, MessageHeader *req);
static void check_all_finished(MessageHeader *req);
static void eliminate_player(MatchPlayerState *mp, const char *reason);
static void perform_elimination(void);
static char* build_round_end_json(void);

//==============================================================================
// HELPER: Reset context
//==============================================================================
static void reset_context(void) {
    pthread_mutex_lock(&g_r3_mutex);
    memset(&g_r3, 0, sizeof(g_r3));
    pthread_mutex_unlock(&g_r3_mutex);
    printf("[Round3] Context reset\n");
}

//==============================================================================
// HELPER: Get states from other PICs
//==============================================================================
static MatchState* get_match(void) {
    if (g_r3.match_id == 0) return NULL;
    return match_get_by_id(g_r3.match_id);
}

static RoundState* get_round(void) {
    MatchState *match = get_match();
    if (!match) return NULL;
    if (g_r3.round_index < 0 || g_r3.round_index >= match->round_count) {
        return NULL;
    }
    return &match->rounds[g_r3.round_index];
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
// HELPER: Local player tracking
//==============================================================================
static R3_PlayerState* find_player(int32_t account_id) {
    for (int i = 0; i < g_r3.player_count; i++) {
        if (g_r3.players[i].account_id == account_id) {
            return &g_r3.players[i];
        }
    }
    return NULL;
}

static R3_PlayerState* add_player(int32_t account_id) {
    R3_PlayerState *existing = find_player(account_id);
    if (existing) return existing;
    if (g_r3.player_count >= MAX_MATCH_PLAYERS) return NULL;
    
    R3_PlayerState *p = &g_r3.players[g_r3.player_count++];
    p->account_id = account_id;
    p->first_spin = 0;
    p->second_spin = 0;
    p->spin_count = 0;
    p->decision_pending = false;
    p->finished = false;
    p->ready = false;
    return p;
}

//==============================================================================
// HELPER: Response/Broadcast
//==============================================================================
static void send_json(int fd, MessageHeader *req, uint16_t cmd, const char *json) {
    if (fd <= 0 || !json) return;
    forward_response(fd, req, cmd, json, (uint32_t)strlen(json));
}

static void broadcast_json(MessageHeader *req, uint16_t cmd, const char *json) {
    if (!json) return;
    for (int i = 0; i < g_r3.player_count; i++) {
        int fd = get_socket(g_r3.players[i].account_id);
        if (fd > 0) {
            forward_response(fd, req, cmd, json, (uint32_t)strlen(json));
        }
    }
}

//==============================================================================
// HELPER: Generate spin result (5-100)
//==============================================================================
static int generate_spin_result(void) {
    // Generate random number between 5 and 100
    // Using multiples of 5 for wheel segments
    int base = (rand() % 20) + 1;  // 1-20
    return base * 5;  // 5, 10, 15, ..., 100
}

//==============================================================================
// HELPER: Calculate bonus according to rules
//==============================================================================
static int calculate_bonus(int first_spin, int second_spin) {
    int total = first_spin;
    if (second_spin > 0) {
        total += second_spin;
    }
    
    // Scoring formula:
    // - If total = 100 or 200: bonus = 100
    // - If total > 100: bonus = amount over 100
    // - Otherwise: bonus = total
    
    if (total == 100 || total == 200) {
        return 100;
    } else if (total > 100) {
        return total - 100;
    } else {
        return total;
    }
}

//==============================================================================
// ELIMINATION LOGIC
//==============================================================================
static void eliminate_player(MatchPlayerState *mp, const char *reason) {
    if (!mp) return;
    
    mp->eliminated = 1;
    mp->eliminated_at_round = g_r3.round_index + 1;
    
    printf("[Round3] Player %d ELIMINATED at round %d (reason: %s, score=%d)\n", 
           mp->account_id, g_r3.round_index + 1, reason, mp->score);
    
    // Send NTF_ELIMINATION to the eliminated player
    UserSession *elim_session = session_get_by_account(mp->account_id);
    int elim_fd = elim_session ? elim_session->socket_fd : -1;
    
    if (elim_fd > 0) {
        cJSON *ntf = cJSON_CreateObject();
        cJSON_AddNumberToObject(ntf, "player_id", mp->account_id);
        cJSON_AddStringToObject(ntf, "reason", reason);
        cJSON_AddNumberToObject(ntf, "round", g_r3.round_index + 1);
        cJSON_AddNumberToObject(ntf, "final_score", mp->score);
        cJSON_AddStringToObject(ntf, "message", "You have been eliminated!");
        
        char *ntf_json = cJSON_PrintUnformatted(ntf);
        cJSON_Delete(ntf);
        
        if (ntf_json) {
            MessageHeader hdr = {0};
            hdr.magic = htons(MAGIC_NUMBER);
            hdr.version = PROTOCOL_VERSION;
            hdr.command = htons(NTF_ELIMINATION);
            hdr.length = htonl((uint32_t)strlen(ntf_json));
            
            send(elim_fd, &hdr, sizeof(hdr), 0);
            send(elim_fd, ntf_json, strlen(ntf_json), 0);
            printf("[Round3] Sent NTF_ELIMINATION to player %d\n", mp->account_id);
            free(ntf_json);
        }
        
        session_mark_lobby(elim_session);
        printf("[Round3] Changed player %d session state to LOBBY\n", mp->account_id);
    }
    
    // Save elimination event to database
    if (mp->match_player_id > 0) {
        MatchState *match = get_match();
        if (match && match->db_match_id > 0) {
            cJSON *event_payload = cJSON_CreateObject();
            cJSON_AddNumberToObject(event_payload, "match_id", match->db_match_id);
            cJSON_AddNumberToObject(event_payload, "player_id", mp->match_player_id);
            cJSON_AddStringToObject(event_payload, "event_type", "ELIMINATED");
            cJSON_AddNumberToObject(event_payload, "round_no", g_r3.round_index + 1);
            cJSON_AddNumberToObject(event_payload, "question_idx", 0);
            
            cJSON *event_result = NULL;
            db_error_t event_err = db_post("match_events", event_payload, &event_result);
            if (event_err == DB_OK) {
                printf("[Round3] Saved elimination event to DB\n");
            }
            cJSON_Delete(event_payload);
            if (event_result) cJSON_Delete(event_result);
        }
        
        // Update player record
        cJSON *player_payload = cJSON_CreateObject();
        cJSON_AddNumberToObject(player_payload, "score", mp->score);
        cJSON_AddBoolToObject(player_payload, "eliminated", true);
        
        char filter[64];
        snprintf(filter, sizeof(filter), "id=eq.%d", mp->match_player_id);
        
        cJSON *player_result = NULL;
        db_patch("match_players", filter, player_payload, &player_result);
        cJSON_Delete(player_payload);
        if (player_result) cJSON_Delete(player_result);
    }
}

static void perform_elimination(void) {
    MatchState *match = get_match();
    if (!match) return;
    
    // MODE_SCORING: No elimination
    if (match->mode == MODE_SCORING) {
        printf("[Round3] Scoring mode - no elimination\n");
        return;
    }
    
    // MODE_ELIMINATION: Eliminate disconnected + lowest scorer
    printf("[Round3] Elimination mode - processing...\n");
    
    // Step 1: Eliminate all disconnected players
    for (int i = 0; i < g_r3.player_count; i++) {
        int32_t acc_id = g_r3.players[i].account_id;
        MatchPlayerState *mp = get_match_player(acc_id);
        if (mp && !mp->eliminated && !is_connected(acc_id)) {
            eliminate_player(mp, "DISCONNECT");
        }
    }
    
    // Step 2: Find lowest scorer among connected players
    typedef struct {
        int32_t account_id;
        int32_t score;
    } ScoreEntry;
    
    ScoreEntry scores[MAX_MATCH_PLAYERS];
    int count = 0;
    
    for (int i = 0; i < g_r3.player_count; i++) {
        MatchPlayerState *mp = get_match_player(g_r3.players[i].account_id);
        if (mp && !mp->eliminated && is_connected(mp->account_id)) {
            scores[count].account_id = mp->account_id;
            scores[count].score = mp->score;
            count++;
        }
    }
    
    if (count < 2) {
        printf("[Round3] Only %d connected players remaining, no further elimination\n", count);
        return;
    }
    
    // Sort ascending by score
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (scores[j].score > scores[j+1].score) {
                ScoreEntry tmp = scores[j];
                scores[j] = scores[j+1];
                scores[j+1] = tmp;
            }
        }
    }
    
    // Check ties at lowest
    int lowest = scores[0].score;
    int tie_count = 0;
    for (int i = 0; i < count; i++) {
        if (scores[i].score == lowest) tie_count++;
    }
    
    if (tie_count >= 2) {
        printf("[Round3] %d players tied at lowest (%d), need bonus round\n", tie_count, lowest);
        return;
    }
    
    // Eliminate lowest scorer
    MatchPlayerState *elim_player = get_match_player(scores[0].account_id);
    if (elim_player) {
        eliminate_player(elim_player, "LOWEST_SCORE");
    }
}

//==============================================================================
// PROCESS: Player finished (calculated bonus)
//==============================================================================
static void process_player_finished(int32_t account_id, MessageHeader *req) {
    R3_PlayerState *p = find_player(account_id);
    if (!p || p->finished) return;
    
    MatchPlayerState *mp = get_match_player(account_id);
    if (!mp) return;
    
    // Calculate bonus
    int bonus = calculate_bonus(p->first_spin, p->second_spin);
    
    // Apply bonus to score
    mp->score += bonus;
    mp->score_delta = bonus;
    
    p->finished = true;
    g_r3.finished_count++;
    
    printf("[Round3] Player %d finished: spin1=%d spin2=%d bonus=%d total_score=%d\n",
           account_id, p->first_spin, p->second_spin, bonus, mp->score);
    
    // Send final result to player
    cJSON *result = cJSON_CreateObject();
    cJSON_AddTrueToObject(result, "success");
    cJSON_AddNumberToObject(result, "first_spin", p->first_spin);
    cJSON_AddNumberToObject(result, "second_spin", p->second_spin);
    cJSON_AddNumberToObject(result, "bonus", bonus);
    cJSON_AddNumberToObject(result, "total_score", mp->score);
    
    char *json = cJSON_PrintUnformatted(result);
    cJSON_Delete(result);
    
    int fd = get_socket(account_id);
    if (fd > 0 && json) {
        send_json(fd, req, OP_S2C_ROUND3_FINAL_RESULT, json);
    }
    if (json) free(json);
    
    // Save to database
    if (mp->match_player_id > 0) {
        RoundState *round = get_round();
        if (round && round->questions[0].question_id > 0) {
            cJSON *ans_payload = cJSON_CreateObject();
            cJSON_AddNumberToObject(ans_payload, "question_id", round->questions[0].question_id);
            cJSON_AddNumberToObject(ans_payload, "player_id", mp->match_player_id);
            cJSON_AddNumberToObject(ans_payload, "score_delta", bonus);
            cJSON_AddNumberToObject(ans_payload, "action_idx", 0);
            
            cJSON *ans_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(ans_obj, "first_spin", p->first_spin);
            cJSON_AddNumberToObject(ans_obj, "second_spin", p->second_spin);
            cJSON_AddNumberToObject(ans_obj, "bonus", bonus);
            cJSON_AddItemToObject(ans_payload, "answer", ans_obj);
            
            cJSON *db_result = NULL;
            db_error_t db_err = db_post("match_answer", ans_payload, &db_result);
            if (db_err == DB_OK) {
                printf("[Round3] Saved bonus to DB: p=%d bonus=%d\n", mp->match_player_id, bonus);
            }
            cJSON_Delete(ans_payload);
            if (db_result) cJSON_Delete(db_result);
        }
        
        // Update player score
        cJSON *player_payload = cJSON_CreateObject();
        cJSON_AddNumberToObject(player_payload, "score", mp->score);
        
        char filter[64];
        snprintf(filter, sizeof(filter), "id=eq.%d", mp->match_player_id);
        
        cJSON *player_result = NULL;
        db_patch("match_players", filter, player_payload, &player_result);
        cJSON_Delete(player_payload);
        if (player_result) cJSON_Delete(player_result);
    }
    
    // Check if all finished
    check_all_finished(req);
}

//==============================================================================
// CHECK: All players finished
//==============================================================================
static void check_all_finished(MessageHeader *req) {
    int expected = 0;
    for (int i = 0; i < g_r3.player_count; i++) {
        MatchPlayerState *mp = get_match_player(g_r3.players[i].account_id);
        if (mp && !mp->eliminated && is_connected(g_r3.players[i].account_id)) {
            expected++;
        }
    }
    
    if (g_r3.finished_count >= expected && expected > 0) {
        printf("[Round3] All players finished\n");
        
        // Perform elimination
        perform_elimination();
        
        // Broadcast final results
        char *end_json = build_round_end_json();
        if (end_json) {
            printf("[Round3] Round end JSON: %s\n", end_json);
            broadcast_json(req, OP_S2C_ROUND3_ALL_FINISHED, end_json);
            free(end_json);
        }
        
        // Mark round as ended
        RoundState *round = get_round();
        if (round) {
            round->status = ROUND_ENDED;
            round->ended_at = time(NULL);
        }
        
        g_r3.is_active = false;
    }
}

//==============================================================================
// HELPER: Build round end result
//==============================================================================
static char* build_round_end_json(void) {
    MatchState *match = get_match();
    if (!match) return NULL;
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddTrueToObject(obj, "success");
    cJSON_AddNumberToObject(obj, "match_id", g_r3.match_id);
    cJSON_AddNumberToObject(obj, "round", 3);
    
    int connected = 0;
    int disconnected = 0;
    for (int i = 0; i < g_r3.player_count; i++) {
        if (is_connected(g_r3.players[i].account_id)) {
            connected++;
        } else {
            disconnected++;
        }
    }
    
    cJSON_AddNumberToObject(obj, "connected_count", connected);
    cJSON_AddNumberToObject(obj, "disconnected_count", disconnected);
    cJSON_AddNumberToObject(obj, "finished_count", g_r3.finished_count);
    cJSON_AddNumberToObject(obj, "player_count", g_r3.player_count);
    
    bool can_continue = (disconnected < 2);
    cJSON_AddBoolToObject(obj, "can_continue", can_continue);
    
    if (disconnected >= 2) {
        cJSON_AddStringToObject(obj, "status_message", "Game ended - too many disconnections");
    } else {
        cJSON_AddStringToObject(obj, "status_message", "Round completed");
    }
    
    // Build rankings (sorted by score descending)
    typedef struct {
        int32_t account_id;
        int32_t score;
    } ScoreEntry;
    
    ScoreEntry scores[MAX_MATCH_PLAYERS];
    int count = 0;
    
    for (int i = 0; i < g_r3.player_count; i++) {
        MatchPlayerState *mp = get_match_player(g_r3.players[i].account_id);
        if (mp) {
            scores[count].account_id = mp->account_id;
            scores[count].score = mp->score;
            count++;
        }
    }
    
    // Sort descending
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (scores[j].score < scores[j+1].score) {
                ScoreEntry tmp = scores[j];
                scores[j] = scores[j+1];
                scores[j+1] = tmp;
            }
        }
    }
    
    cJSON *players = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        MatchPlayerState *mp = get_match_player(scores[i].account_id);
        if (!mp) continue;
        
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "id", mp->account_id);
        cJSON_AddNumberToObject(p, "rank", i + 1);
        cJSON_AddNumberToObject(p, "score", mp->score);
        cJSON_AddBoolToObject(p, "connected", is_connected(mp->account_id));
        cJSON_AddBoolToObject(p, "eliminated", mp->eliminated != 0);
        
        char name[32];
        snprintf(name, sizeof(name), "Player%d", mp->account_id);
        cJSON_AddStringToObject(p, "name", name);
        
        cJSON_AddItemToArray(players, p);
    }
    
    cJSON_AddItemToObject(obj, "players", players);
    cJSON_AddItemToObject(obj, "rankings", cJSON_Duplicate(players, 1));
    
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    return json;
}

//==============================================================================
// HANDLER: Player Ready (OP_C2S_ROUND3_PLAYER_READY)
//==============================================================================
static void handle_player_ready(int fd, MessageHeader *req, const char *payload) {
    if (req->length < 8) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid payload\"}");
        return;
    }
    
    uint32_t match_id, player_id;
    memcpy(&match_id, payload, 4);
    memcpy(&player_id, payload + 4, 4);
    match_id = ntohl(match_id);
    player_id = ntohl(player_id);
    
    UserSession *session = session_get_by_socket(fd);
    int32_t account_id = session ? session->account_id : (int32_t)player_id;
    
    printf("[Round3] Player ready: match=%u account=%d\n", match_id, account_id);
    
    MatchState *match = match_get_by_id(match_id);
    if (!match) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Match not found\"}");
        return;
    }
    
    // Check player is in match and not eliminated
    MatchPlayerState *mp = NULL;
    for (int i = 0; i < match->player_count; i++) {
        if (match->players[i].account_id == account_id) {
            mp = &match->players[i];
            break;
        }
    }
    
    if (!mp) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Not in match\"}");
        return;
    }
    
    if (mp->eliminated) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Player eliminated\"}");
        return;
    }
    
    // Initialize context if new match/round
    if (match_id != g_r3.match_id) {
        reset_context();
        g_r3.match_id = match_id;
        g_r3.round_index = match->current_round_idx;
    }
    
    R3_PlayerState *p = add_player(account_id);
    if (!p) {
        send_json(fd, req, ERR_ROOM_FULL, "{\"success\":false,\"error\":\"Round full\"}");
        return;
    }
    
    // Check for reconnection during active round
    RoundState *round = get_round();
    if (round && round->status == ROUND_PLAYING) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddTrueToObject(obj, "success");
        cJSON_AddTrueToObject(obj, "reconnected");
        cJSON_AddNumberToObject(obj, "current_score", mp->score);
        cJSON_AddNumberToObject(obj, "spin_count", p->spin_count);
        cJSON_AddNumberToObject(obj, "first_spin", p->first_spin);
        cJSON_AddBoolToObject(obj, "decision_pending", p->decision_pending);
        
        char *json = cJSON_PrintUnformatted(obj);
        cJSON_Delete(obj);
        send_json(fd, req, OP_S2C_ROUND3_READY_STATUS, json);
        if (json) free(json);
        
        // If player needs to make decision, prompt again
        if (p->decision_pending) {
            cJSON *prompt = cJSON_CreateObject();
            cJSON_AddTrueToObject(prompt, "success");
            cJSON_AddStringToObject(prompt, "message", "Make your decision: Continue or Stop?");
            cJSON_AddNumberToObject(prompt, "first_spin", p->first_spin);
            
            char *prompt_json = cJSON_PrintUnformatted(prompt);
            cJSON_Delete(prompt);
            send_json(fd, req, OP_S2C_ROUND3_DECISION_ACK, prompt_json);
            if (prompt_json) free(prompt_json);
        }
        return;
    }
    
    // Mark ready
    if (!p->ready) {
        p->ready = true;
        g_r3.ready_count++;
    }
    
    // Broadcast ready status
    cJSON *status = cJSON_CreateObject();
    cJSON_AddTrueToObject(status, "success");
    cJSON_AddNumberToObject(status, "ready_count", g_r3.ready_count);
    cJSON_AddNumberToObject(status, "player_count", g_r3.player_count);
    
    int expected = 0;
    for (int i = 0; i < match->player_count; i++) {
        if (!match->players[i].eliminated) expected++;
    }
    
    cJSON_AddNumberToObject(status, "required_players", expected);
    
    cJSON *players = cJSON_CreateArray();
    for (int i = 0; i < g_r3.player_count; i++) {
        cJSON *p_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(p_obj, "account_id", g_r3.players[i].account_id);
        cJSON_AddBoolToObject(p_obj, "ready", g_r3.players[i].ready);
        cJSON_AddItemToArray(players, p_obj);
    }
    cJSON_AddItemToObject(status, "players", players);
    
    char *json = cJSON_PrintUnformatted(status);
    cJSON_Delete(status);
    broadcast_json(req, OP_S2C_ROUND3_READY_STATUS, json);
    if (json) free(json);
    
    // Check if all ready
    printf("[Round3] Check start: ready=%d, expected=%d, round_status=%d\n",
           g_r3.ready_count, expected, round ? round->status : -1);
    
    if (g_r3.ready_count >= expected && expected > 0 && round && round->status == ROUND_PENDING) {
        printf("[Round3] All ready, starting round\n");
        
        round->status = ROUND_PLAYING;
        round->started_at = time(NULL);
        round->current_question_idx = 0;
        g_r3.is_active = true;
        
        cJSON *start = cJSON_CreateObject();
        cJSON_AddTrueToObject(start, "success");
        cJSON_AddNumberToObject(start, "match_id", match_id);
        cJSON_AddNumberToObject(start, "round", 3);
        cJSON_AddNumberToObject(start, "player_count", g_r3.player_count);
        cJSON_AddNumberToObject(start, "max_spins", MAX_SPINS);
        cJSON_AddNumberToObject(start, "spin_min", SPIN_MIN_VALUE);
        cJSON_AddNumberToObject(start, "spin_max", SPIN_MAX_VALUE);
        
        char *start_json = cJSON_PrintUnformatted(start);
        cJSON_Delete(start);
        broadcast_json(req, OP_S2C_ROUND3_ALL_READY, start_json);
        if (start_json) free(start_json);
        
        // Prompt first spin for all players
        cJSON *prompt = cJSON_CreateObject();
        cJSON_AddTrueToObject(prompt, "success");
        cJSON_AddStringToObject(prompt, "message", "Spin the wheel!");
        cJSON_AddNumberToObject(prompt, "spin_number", 1);
        
        char *prompt_json = cJSON_PrintUnformatted(prompt);
        cJSON_Delete(prompt);
        broadcast_json(req, OP_S2C_ROUND3_DECISION_ACK, prompt_json);
        if (prompt_json) free(prompt_json);
    }
}

//==============================================================================
// HANDLER: Spin (OP_C2S_ROUND3_SPIN)
//==============================================================================
static void handle_spin(int fd, MessageHeader *req, const char *payload) {
    if (req->length < 4) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid payload\"}");
        return;
    }
    
    uint32_t match_id;
    memcpy(&match_id, payload, 4);
    match_id = ntohl(match_id);
    
    if (match_id != g_r3.match_id) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid match\"}");
        return;
    }
    
    RoundState *round = get_round();
    if (!round || round->status != ROUND_PLAYING) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Round not active\"}");
        return;
    }
    
    UserSession *session = session_get_by_socket(fd);
    if (!session) {
        send_json(fd, req, ERR_NOT_LOGGED_IN, "{\"success\":false,\"error\":\"Not logged in\"}");
        return;
    }
    
    MatchPlayerState *mp = get_match_player(session->account_id);
    if (!mp) {
        send_json(fd, req, ERR_SERVER_ERROR, "{\"success\":false,\"error\":\"Player not in match\"}");
        return;
    }
    
    if (mp->eliminated) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Player eliminated\"}");
        return;
    }
    
    R3_PlayerState *p = find_player(session->account_id);
    if (!p) {
        send_json(fd, req, ERR_SERVER_ERROR, "{\"success\":false,\"error\":\"Player not in round\"}");
        return;
    }
    
    // Check spin count
    if (p->spin_count >= MAX_SPINS) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Maximum spins reached\"}");
        return;
    }
    
    // Generate spin result
    int spin_result = generate_spin_result();
    p->spin_count++;
    
    if (p->spin_count == 1) {
        p->first_spin = spin_result;
        p->decision_pending = true;
    } else if (p->spin_count == 2) {
        p->second_spin = spin_result;
        p->decision_pending = false;
        // Second spin means player chose to continue, calculate bonus immediately
        process_player_finished(session->account_id, req);
    }
    
    printf("[Round3] Player %d spin %d: result=%d\n", session->account_id, p->spin_count, spin_result);
    
    // Send spin result
    cJSON *result = cJSON_CreateObject();
    cJSON_AddTrueToObject(result, "success");
    cJSON_AddNumberToObject(result, "spin_number", p->spin_count);
    cJSON_AddNumberToObject(result, "result", spin_result);
    cJSON_AddNumberToObject(result, "first_spin", p->first_spin);
    cJSON_AddNumberToObject(result, "second_spin", p->second_spin);
    cJSON_AddBoolToObject(result, "decision_pending", p->decision_pending);
    
    char *json = cJSON_PrintUnformatted(result);
    cJSON_Delete(result);
    send_json(fd, req, OP_S2C_ROUND3_SPIN_RESULT, json);
    if (json) free(json);
    
    // If first spin, prompt for decision
    if (p->spin_count == 1 && p->decision_pending) {
        cJSON *prompt = cJSON_CreateObject();
        cJSON_AddTrueToObject(prompt, "success");
        cJSON_AddStringToObject(prompt, "message", "Continue or Stop?");
        cJSON_AddNumberToObject(prompt, "first_spin", p->first_spin);
        cJSON_AddNumberToObject(prompt, "timeout_ms", DECISION_TIMEOUT_MS);
        
        char *prompt_json = cJSON_PrintUnformatted(prompt);
        cJSON_Delete(prompt);
        send_json(fd, req, OP_S2C_ROUND3_DECISION_ACK, prompt_json);
        if (prompt_json) free(prompt_json);
    }
}

//==============================================================================
// HANDLER: Decision (OP_C2S_ROUND3_DECISION)
// Payload: match_id (4) + decision (1) where 0=Stop, 1=Continue
//==============================================================================
static void handle_decision(int fd, MessageHeader *req, const char *payload) {
    if (req->length < 5) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid payload\"}");
        return;
    }
    
    uint32_t match_id;
    uint8_t decision;
    memcpy(&match_id, payload, 4);
    memcpy(&decision, payload + 4, 1);
    match_id = ntohl(match_id);
    
    if (match_id != g_r3.match_id) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid match\"}");
        return;
    }
    
    RoundState *round = get_round();
    if (!round || round->status != ROUND_PLAYING) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Round not active\"}");
        return;
    }
    
    UserSession *session = session_get_by_socket(fd);
    if (!session) {
        send_json(fd, req, ERR_NOT_LOGGED_IN, "{\"success\":false,\"error\":\"Not logged in\"}");
        return;
    }
    
    MatchPlayerState *mp = get_match_player(session->account_id);
    if (!mp) {
        send_json(fd, req, ERR_SERVER_ERROR, "{\"success\":false,\"error\":\"Player not in match\"}");
        return;
    }
    
    if (mp->eliminated) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Player eliminated\"}");
        return;
    }
    
    R3_PlayerState *p = find_player(session->account_id);
    if (!p) {
        send_json(fd, req, ERR_SERVER_ERROR, "{\"success\":false,\"error\":\"Player not in round\"}");
        return;
    }
    
    if (!p->decision_pending) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"No decision pending\"}");
        return;
    }
    
    if (p->spin_count != 1) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid spin count\"}");
        return;
    }
    
    p->decision_pending = false;
    
    // Send acknowledgment
    cJSON *ack = cJSON_CreateObject();
    cJSON_AddTrueToObject(ack, "success");
    cJSON_AddStringToObject(ack, "decision", decision == 1 ? "continue" : "stop");
    
    char *json = cJSON_PrintUnformatted(ack);
    cJSON_Delete(ack);
    send_json(fd, req, OP_S2C_ROUND3_DECISION_ACK, json);
    if (json) free(json);
    
    if (decision == 0) {
        // Stop: Calculate bonus from first spin only
        printf("[Round3] Player %d chose to STOP\n", session->account_id);
        process_player_finished(session->account_id, req);
    } else {
        // Continue: Prompt for second spin
        printf("[Round3] Player %d chose to CONTINUE\n", session->account_id);
        
        cJSON *prompt = cJSON_CreateObject();
        cJSON_AddTrueToObject(prompt, "success");
        cJSON_AddStringToObject(prompt, "message", "Spin again!");
        cJSON_AddNumberToObject(prompt, "spin_number", 2);
        cJSON_AddNumberToObject(prompt, "first_spin", p->first_spin);
        
        char *prompt_json = cJSON_PrintUnformatted(prompt);
        cJSON_Delete(prompt);
        send_json(fd, req, OP_S2C_ROUND3_DECISION_ACK, prompt_json);
        if (prompt_json) free(prompt_json);
    }
}

//==============================================================================
// DISCONNECT HANDLER
//==============================================================================
void handle_round3_disconnect(int client_fd) {
    UserSession *session = session_get_by_socket(client_fd);
    if (!session) return;
    
    int32_t account_id = session->account_id;
    R3_PlayerState *p = find_player(account_id);
    if (!p) return;
    
    printf("[Round3] Player %d disconnected\n", account_id);
    
    MatchPlayerState *mp = get_match_player(account_id);
    if (mp) {
        mp->connected = 0;
    }
    
    // If player was in middle of round, mark as finished with 0 bonus
    if (!p->finished && p->spin_count > 0) {
        process_player_finished(account_id, NULL);
    }
}

//==============================================================================
// MAIN DISPATCHER
//==============================================================================
void handle_round3(int client_fd, MessageHeader *req_header, const char *payload) {
    printf("[Round3] cmd=0x%04X len=%u\n", req_header->command, req_header->length);
    
    switch (req_header->command) {
        case OP_C2S_ROUND3_READY:
        case OP_C2S_ROUND3_PLAYER_READY:
            handle_player_ready(client_fd, req_header, payload);
            break;
        case OP_C2S_ROUND3_SPIN:
            handle_spin(client_fd, req_header, payload);
            break;
        case OP_C2S_ROUND3_DECISION:
            handle_decision(client_fd, req_header, payload);
            break;
        default:
            printf("[Round3] Unknown cmd: 0x%04X\n", req_header->command);
            break;
    }
}
