/**
 * round1_handler.c - Round 1: MCQ (Multiple Choice Questions)
 * Manages round execution flow for the quiz round.
 * 
 * STATE USAGE:
 * - Score: MatchPlayerState.score - READ/UPDATE via get_match_player()
 * - Eliminated: MatchPlayerState.eliminated - READ/UPDATE
 * - Connected: UserSession.state - READ via session_get_by_account()
 * - Current question: RoundState.current_question_idx - READ/UPDATE
 * - Question data: RoundState.question_data[] - READ only
 * 
 * LOCAL TRACKING (Round-specific):
 * - answered_current: Has player answered the CURRENT question?
 * - ready: Is player ready to start the round?
 * 
 * Flow:
 * 1. All players ready → Round starts
 * 2. Server sends current question to all players
 * 3. Players answer → Server calculates score, updates MatchPlayerState
 * 4. When all answered → Move to next question
 * 5. After last question → Check elimination, update MatchPlayerState.eliminated
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>                     // For sleep()

#include "handlers/round1_handler.h"
#include "handlers/session_manager.h"   // UserSession management
#include "handlers/match_manager.h"     // MatchState management
#include "handlers/start_game_handler.h" // State definitions
#include "handlers/bonus_handler.h"     // Bonus round for ties
#include "handlers/end_game_handler.h"  // End game handling
#include "transport/room_manager.h"
#include "db/core/db_client.h"          // Direct DB access
#include "db/repo/match_repo.h"         // For db_match_event_insert, db_match_answer_insert
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include <cjson/cJSON.h>
#include <pthread.h>                    // For timeout timer

//==============================================================================
// CONSTANTS
//==============================================================================
#define TIME_PER_QUESTION   15000 // 10 seconds in milliseconds
#define MAX_SCORE_PER_Q     200   // Max score per question
#define MIN_SCORE_PER_Q     100   // Min score for correct answer

//==============================================================================
// ROUND 1 EXECUTION CONTEXT
// 
// ONLY tracks round-specific execution state that does NOT exist elsewhere:
// - answered_current: Has player answered the current question? (per-question)
// - ready: Is player ready to start? (per-round)
// 
// Uses from other modules (NOT duplicating):
// - current_question_idx → RoundState.current_question_idx
// - score → MatchPlayerState.score
// - eliminated → MatchPlayerState.eliminated
// - connected → UserSession.state
//==============================================================================

typedef struct {
    int32_t account_id;         // Links to MatchPlayerState & UserSession
    bool    answered_current;   // Has answered CURRENT question (reset each question)
    bool    ready;              // Ready for round to start
} R1_PlayerAnswer;

typedef struct {
    uint32_t match_id;          // Links to MatchState
    int      round_index;       // Which round (0-based)
    bool     is_active;         // Is round currently running
    
    R1_PlayerAnswer players[MAX_MATCH_PLAYERS];
    int      player_count;
    int      ready_count;
    
    // Timer for question timeout
    time_t   question_start_time;  // When current question started
    int      current_timer_q_idx;  // Question index for timer
    pthread_t timer_thread;        // Timer thread handle
    bool     timer_running;        // Is timer active
} R1_Context;

// Global round 1 context (in-memory only)
static R1_Context g_r1 = {0};
static pthread_mutex_t g_r1_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declaration for timer thread
void advance_to_next_question(MessageHeader *req);
static void broadcast_json(MessageHeader *req, uint16_t cmd, const char *json);
static bool perform_elimination(void);
//==============================================================================
// HELPER: Reset context
//==============================================================================

static void reset_context(void) {
    pthread_mutex_lock(&g_r1_mutex);
    g_r1.timer_running = false;
    pthread_mutex_unlock(&g_r1_mutex);
    
    memset(&g_r1, 0, sizeof(g_r1));
    printf("[Round1] Context reset\n");
}

//==============================================================================
// TIMER: Question timeout handler
// Runs in separate thread, auto-advances after TIME_PER_QUESTION
//==============================================================================

static void* question_timer_thread(void *arg) {
    (void)arg;
    
    int target_q_idx = g_r1.current_timer_q_idx;
    uint32_t target_match = g_r1.match_id;
    
    printf("[Round1-Timer] Started for question %d (timeout: %dms)\n", 
           target_q_idx, TIME_PER_QUESTION);
    
    // Sleep for question duration (convert ms to seconds)
    int sleep_time = TIME_PER_QUESTION / 1000;
    for (int i = 0; i < sleep_time; i++) {
        sleep(1);
        
        pthread_mutex_lock(&g_r1_mutex);
        bool still_running = g_r1.timer_running;
        bool same_question = (g_r1.current_timer_q_idx == target_q_idx && 
                             g_r1.match_id == target_match);
        pthread_mutex_unlock(&g_r1_mutex);
        
        if (!still_running || !same_question) {
            printf("[Round1-Timer] Cancelled (question already advanced)\n");
            return NULL;
}
    }
    
    // Check if we should still advance
    pthread_mutex_lock(&g_r1_mutex);
    bool should_advance = g_r1.timer_running && 
                         g_r1.current_timer_q_idx == target_q_idx &&
                         g_r1.match_id == target_match &&
                         g_r1.is_active;
    pthread_mutex_unlock(&g_r1_mutex);
    
    if (should_advance) {
        printf("[Round1-Timer] ⏰ TIMEOUT! Auto-advancing from question %d\n", target_q_idx);
  
        // Mark all non-answered players as having answered (with 0 score)
        for (int i = 0; i < g_r1.player_count; i++) {
            if (!g_r1.players[i].answered_current) {
                g_r1.players[i].answered_current = true;
                printf("[Round1-Timer] Player %d did not answer in time\n", 
                       g_r1.players[i].account_id);
            }
        }
        
        // Advance to next question (using a dummy header)
        MessageHeader dummy = {0};
        advance_to_next_question(&dummy);
    }
    
    return NULL;
    }

static void start_question_timer(int q_idx) {
    pthread_mutex_lock(&g_r1_mutex);
    
    // Stop previous timer if running
    g_r1.timer_running = false;
    
    // Set up new timer
    g_r1.question_start_time = time(NULL);
    g_r1.current_timer_q_idx = q_idx;
    g_r1.timer_running = true;
    
    pthread_mutex_unlock(&g_r1_mutex);
    
    // Create timer thread (detached - don't need to join)
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    if (pthread_create(&g_r1.timer_thread, &attr, question_timer_thread, NULL) != 0) {
        printf("[Round1-Timer] Warning: Failed to create timer thread\n");
    }
    
    pthread_attr_destroy(&attr);
}

static void stop_question_timer(void) {
    pthread_mutex_lock(&g_r1_mutex);
    g_r1.timer_running = false;
    pthread_mutex_unlock(&g_r1_mutex);
}

//==============================================================================
// HELPER: Get states from other PICs
//==============================================================================

/**
 * Get MatchState from match_manager
 */
static MatchState* get_match(void) {
    if (g_r1.match_id == 0) return NULL;
    return match_get_by_id(g_r1.match_id);
}

/**
 * Get current RoundState from MatchState
 */
static RoundState* get_round(void) {
    MatchState *match = get_match();
    if (!match) return NULL;
    if (g_r1.round_index < 0 || g_r1.round_index >= match->round_count) {
        return NULL;
    }
    return &match->rounds[g_r1.round_index];
}

/**
 * Get MatchPlayerState by account_id
 */
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

/**
 * Check if player is connected using UserSession
 */
static bool is_connected(int32_t account_id) {
    UserSession *session = session_get_by_account(account_id);
    if (!session) return false;
    return session->state != SESSION_PLAYING_DISCONNECTED;
}

/**
 * Get socket_fd for player from UserSession
 */
static int get_socket(int32_t account_id) {
    UserSession *session = session_get_by_account(account_id);
    if (!session || session->state == SESSION_PLAYING_DISCONNECTED) {
        return -1;
    }
    return session->socket_fd;
    }
    
//==============================================================================
// HELPER: Local answer tracking
//==============================================================================

static R1_PlayerAnswer* find_player(int32_t account_id) {
    for (int i = 0; i < g_r1.player_count; i++) {
        if (g_r1.players[i].account_id == account_id) {
            return &g_r1.players[i];
        }
    }
    return NULL;
}

static R1_PlayerAnswer* add_player(int32_t account_id) {
    R1_PlayerAnswer *existing = find_player(account_id);
    if (existing) return existing;
    
    if (g_r1.player_count >= MAX_MATCH_PLAYERS) return NULL;
    
    R1_PlayerAnswer *p = &g_r1.players[g_r1.player_count++];
    p->account_id = account_id;
    p->answered_current = false;
    p->ready = false;
    return p;
}

/**
 * Reset answered_current for all players (called when moving to next question)
 */
static void reset_answered_flags(void) {
    for (int i = 0; i < g_r1.player_count; i++) {
        g_r1.players[i].answered_current = false;
    }
}

/**
 * Count how many have answered current question
 */
static int count_answered(void) {
    int count = 0;
    for (int i = 0; i < g_r1.player_count; i++) {
        if (g_r1.players[i].answered_current) {
                count++;
        }
    }
  return count;
}

/**
 * Count connected players (using UserSession)
 */
static int count_connected(void) {
    int count = 0;
    for (int i = 0; i < g_r1.player_count; i++) {
        if (is_connected(g_r1.players[i].account_id)) {
                count++;
        }
    }
    return count;
}

/**
 * Count disconnected players
 */
static int count_disconnected(void) {
    return g_r1.player_count - count_connected();
}

//==============================================================================
// HELPER: Score calculation
//==============================================================================

static int calc_score(bool correct, uint32_t time_ms) {
    if (!correct) return 0;
    
    float t = (float)time_ms / 1000.0f;
    int score = (int)(MAX_SCORE_PER_Q - (t / 15.0f) * 100);
    if (score < MIN_SCORE_PER_Q) score = MIN_SCORE_PER_Q;
    if (score > MAX_SCORE_PER_Q) score = MAX_SCORE_PER_Q;
    return (score / 10) * 10;
}

//==============================================================================
// HELPER: Get correct answer from question data
//==============================================================================

static int get_correct_index(int q_idx) {
    RoundState *round = get_round();
    if (!round || q_idx < 0 || q_idx >= round->question_count) return -1;
    
    MatchQuestion *mq = &round->question_data[q_idx];
    if (!mq->json_data) return -1;
    
    cJSON *root = cJSON_Parse(mq->json_data);
    if (!root) return -1;
    
    // Question data is nested inside "data" field from DB
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!data) {
        // Fallback: maybe data is at root level
        data = root;
    }
    
    int correct = -1;
    
    // 1. Check "answer" field (most common in DB)
    cJSON *ans = cJSON_GetObjectItem(data, "answer");
    if (ans && cJSON_IsNumber(ans)) {
        correct = ans->valueint;
        printf("[Round1] Q%d: answer = %d\n", q_idx, correct);
    }
    // 2. Check "correct_index" field (number)
    else {
        cJSON *cidx = cJSON_GetObjectItem(data, "correct_index");
        if (cidx && cJSON_IsNumber(cidx)) {
            correct = cidx->valueint;
            printf("[Round1] Q%d: correct_index = %d\n", q_idx, correct);
        } 
        // 3. Check "correct_answer" field - can be NUMBER or STRING
        else {
            cJSON *cans = cJSON_GetObjectItem(data, "correct_answer");
            if (cans) {
                if (cJSON_IsNumber(cans)) {
                    correct = cans->valueint;
                    printf("[Round1] Q%d: correct_answer (number) = %d\n", q_idx, correct);
                } 
                else if (cJSON_IsString(cans)) {
                    // correct_answer is a STRING - search in choices
                    cJSON *choices = cJSON_GetObjectItem(data, "choices");
                    if (choices && cJSON_IsArray(choices)) {
                        for (int j = 0; j < cJSON_GetArraySize(choices); j++) {
                            cJSON *c = cJSON_GetArrayItem(choices, j);
                            if (c && cJSON_IsString(c) && strcmp(c->valuestring, cans->valuestring) == 0) {
                                correct = j;
                                printf("[Round1] Q%d: correct_answer (string match) = %d\n", q_idx, correct);
                                break;
    }
  }
                    }
                }
            }
        }
    }
    
    cJSON_Delete(root);
    return correct;
}

static void end_game_now(MessageHeader *req, const char *reason) {
    MatchState *match = get_match();
    if (!match) return;
    
    printf("[Round1] END GAME: %s\n", reason);
    
    // Use official trigger_end_game which handles all cleanup and notifications
    trigger_end_game(match->runtime_match_id, -1); // -1 = no bonus winner
}

// returns true nếu match kết thúc tại đây (END GAME), false nếu còn tiếp
static bool r1_finalize_round(MessageHeader *req, bool *out_bonus_triggered) {
    printf("[Round1] r1_finalize_round() ENTERED\n");
    
    MatchState *match = get_match();
    if (!match) {
        printf("[Round1] r1_finalize_round() - No match, returning true\n");
        return true;
    }

    uint32_t match_id = match->runtime_match_id;
    printf("[Round1] r1_finalize_round() - match_id=%u\n", match_id);
    
    // Initialize output parameter
    if (out_bonus_triggered) *out_bonus_triggered = false;

    // END ROUND → CHECK DISCONNECTED
    cleanup_disconnected_and_forfeit(match_id);

    // ACTIVE PLAYERS < 2 ?
    int active = count_active_players(match_id);
    printf("[Round1] r1_finalize_round() - active players=%d\n", active);
    
    if (active < 2) {
        printf("[Round1] r1_finalize_round() - Active < 2, ending game\n");
        end_game_now(req, "ACTIVE_PLAYERS_LT_2");
        return true;
    }

    // MODE_SCORING ?
    if (match->mode == MODE_SCORING) {
        printf("[Round1] r1_finalize_round() - MODE_SCORING branch\n");
        // NEXT ROUND / END GAME
        if (match->current_round_idx >= match->round_count - 1) {
            end_game_now(req, "ALL_ROUNDS_COMPLETED");
            return true;
        }

        // next round
        match->current_round_idx++;
        RoundState *next_round = &match->rounds[match->current_round_idx];
        next_round->status = ROUND_PENDING;
        next_round->current_question_idx = 0;
        return false;
    }

    // NO (ELIMINATION) → CHECK LOWEST SCORE
    printf("[Round1] r1_finalize_round() - Calling perform_elimination()\n");
    bool bonus_triggered = perform_elimination(); // chỉ làm tie/lowest/eliminate, KHÔNG end game ở đây
    printf("[Round1] r1_finalize_round() - perform_elimination() returned: %d\n", bonus_triggered);
    
    if (bonus_triggered) {
        // BONUS ROUND: bonus handler sẽ tự điều hướng
        if (out_bonus_triggered) *out_bonus_triggered = true;
        printf("[Round1] Bonus triggered, setting output flag\n");
        printf("[Round1] r1_finalize_round() - RETURNING false (bonus triggered)\n");
        return false;
    }

    // ELIMINATE xong → ACTIVE PLAYERS < 2 ?
    active = count_active_players(match_id);
    if (active < 2) {
        end_game_now(req, "ACTIVE_PLAYERS_LT_2_AFTER_ELIM");
        return true;
    }

    // NO → NEXT ROUND
    if (match->current_round_idx >= match->round_count - 1) {
        end_game_now(req, "ALL_ROUNDS_COMPLETED");
        return true;
    }

    match->current_round_idx++;
    RoundState *next_round = &match->rounds[match->current_round_idx];
    next_round->status = ROUND_PENDING;
    next_round->current_question_idx = 0;

    return false;
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
    for (int i = 0; i < g_r1.player_count; i++) {
        int fd = get_socket(g_r1.players[i].account_id);
        if (fd > 0) {
            forward_response(fd, req, cmd, json, (uint32_t)strlen(json));
        }
    }
}

//==============================================================================
// HELPER: Build question payload from RoundState.question_data
//==============================================================================

static char* build_question_json(int q_idx) {
    RoundState *round = get_round();
    if (!round || q_idx < 0 || q_idx >= round->question_count) return NULL;
    
    MatchQuestion *mq = &round->question_data[q_idx];
    if (!mq->json_data) return NULL;
  
    cJSON *root = cJSON_Parse(mq->json_data);
    if (!root) return NULL;
    
    // Question data is nested inside "data" field from DB
    // Structure: {id, type, data: {content, choices, answer, image}}
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!data) {
        // Fallback: maybe data is at root level
        data = root;
    }
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddTrueToObject(obj, "success");  // Frontend checks this
    cJSON_AddNumberToObject(obj, "question_idx", q_idx);
    cJSON_AddNumberToObject(obj, "total_questions", round->question_count);
    cJSON_AddNumberToObject(obj, "time_limit_ms", TIME_PER_QUESTION);
    
    // ⭐ Option 3: Send server timestamp for client-side time sync
    // Client uses this to calculate accurate time_left
    cJSON_AddNumberToObject(obj, "start_timestamp", (double)g_r1.question_start_time);
      
    // Copy from question data - try both "question" and "content"
    cJSON *text = cJSON_GetObjectItem(data, "question");
    if (!text) text = cJSON_GetObjectItem(data, "content");
    if (text && cJSON_IsString(text)) {
        cJSON_AddStringToObject(obj, "question", text->valuestring);
    }
    
    cJSON *choices = cJSON_GetObjectItem(data, "choices");
    if (choices && cJSON_IsArray(choices)) {
        cJSON_AddItemToObject(obj, "choices", cJSON_Duplicate(choices, 1));
    }
    
    cJSON *img = cJSON_GetObjectItem(data, "image");
    if (!img) img = cJSON_GetObjectItem(data, "product_image");
    if (img && cJSON_IsString(img)) {
        cJSON_AddStringToObject(obj, "product_image", img->valuestring);
    }
  
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    cJSON_Delete(root);
  return json;
}

//==============================================================================
// ELIMINATION LOGIC
// Updates MatchPlayerState.eliminated
// Only runs for MODE_ELIMINATION, skips for MODE_SCORING
//==============================================================================

typedef struct {
    int32_t account_id;
    int32_t score;
} ScoreEntry;

/**
 * Helper: Mark player as eliminated, notify them, and redirect to lobby
 */
static void eliminate_player(MatchPlayerState *mp, const char *reason) {
    if (!mp) return;

    // 1. Mark eliminated
    mp->eliminated = 1;
    mp->eliminated_at_round = g_r1.round_index + 1;

    printf("[Round1] Player %d ELIMINATED (reason=%s)\n",
           mp->account_id, reason);

    // 2. Get session
    UserSession *s = session_get_by_account(mp->account_id);
    if (!s) return;

    int fd = s->socket_fd;

    // 3. Send NTF_ELIMINATION
    if (fd > 0) {
        uint32_t room_id = room_find_by_player_fd(fd);
        cJSON *ntf = cJSON_CreateObject();
        cJSON_AddNumberToObject(ntf, "player_id", mp->account_id);
        cJSON_AddNumberToObject(ntf, "room_id", room_id);
        cJSON_AddStringToObject(ntf, "event", "ELIMINATED");
        cJSON_AddStringToObject(ntf, "reason", reason);
        cJSON_AddNumberToObject(ntf, "round", g_r1.round_index + 1);
        cJSON_AddNumberToObject(ntf, "final_score", mp->score);

        char *json = cJSON_PrintUnformatted(ntf);
        cJSON_Delete(ntf);

        MessageHeader hdr = {0};
        hdr.magic   = htons(MAGIC_NUMBER);
        hdr.version = PROTOCOL_VERSION;
        hdr.command = htons(NTF_ELIMINATION);
        hdr.length  = htonl((uint32_t)strlen(json));

        send(fd, &hdr, sizeof(hdr), 0);
        send(fd, json, strlen(json), 0);
        free(json);
    }

    // 4. ĐƯA SESSION VỀ LOBBY (GIỐNG FORFEIT)
    session_mark_lobby(s);

    // 5. RESET READY + BROADCAST ROOM
    uint32_t room_id = room_find_by_player_fd(fd);
    if (room_id > 0) {
        room_set_ready(room_id, fd, false);
        broadcast_player_list(room_id);
    }

    printf("[Round1] Player %d moved back to room %u\n",
           mp->account_id, room_id);
}

// NOTE: forfeited field is reserved for player-initiated forfeit (FORFEIT button)
// Disconnect handling uses eliminated for BOTH modes

// Returns true if bonus round was triggered (caller should NOT advance to next round)
static bool perform_elimination(void) {
    MatchState *match = get_match();
    if (!match) return false;
    
    //==========================================================================
    // MODE_SCORING: Disconnected players → 0 điểm round này, KHÔNG eliminated
    // - Block đến cuối ROUND (không phải cuối game)
    // - Không có event ELIMINATED
    // - Round sau có thể reconnect và chơi tiếp
    // NOTE: Logic block trong round được xử lý bởi is_connected() check
    //==========================================================================
    if (match->mode == MODE_SCORING) {
        printf("[Round1] Scoring mode - disconnected players get 0 points this round\n");
        
        // Chỉ log, không set eliminated
        // Disconnected players đã nhận 0 điểm vì timeout tự động
        // Họ có thể reconnect và chơi round tiếp theo
        for (int i = 0; i < g_r1.player_count; i++) {
            int32_t acc_id = g_r1.players[i].account_id;
            if (!is_connected(acc_id)) {
                printf("[Round1] Player %d disconnected - 0 points for this round (can rejoin next round)\n", acc_id);
            }
        }
        
        printf("[Round1] Scoring mode - no elimination (ranking only)\n");
        return false;
    }
    
    //==========================================================================
    // MODE_ELIMINATION: Disconnected players → bị loại NGAY LẬP TỨC
    //==========================================================================
    
    // STEP 2: Đếm connected players còn lại chưa bị eliminated
    ScoreEntry scores[MAX_MATCH_PLAYERS];
    int count = 0;
    
    for (int i = 0; i < g_r1.player_count; i++) {
        MatchPlayerState *mp = get_match_player(g_r1.players[i].account_id);
        // Chỉ tính players: connected AND chưa eliminated
        if (mp && !mp->eliminated && is_connected(mp->account_id)) {
            scores[count].account_id = mp->account_id;
            scores[count].score = mp->score;
            count++;
        }
    }
    
    // Nếu còn < 2 connected players → không elimination thêm
    if (count < 2) {
        printf("[Round1] Only %d connected players remaining, no further elimination\n", count);
        return false;
    }
    
    // STEP 3: Sort ascending by score
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (scores[j].score > scores[j+1].score) {
                ScoreEntry tmp = scores[j];
                scores[j] = scores[j+1];
                scores[j+1] = tmp;
            }
        }
    }
    
    // STEP 4: Check ties at lowest
    int lowest = scores[0].score;
    int tie_count = 0;
    for (int i = 0; i < count; i++) {
        if (scores[i].score == lowest) tie_count++;
    }
    
    // Rule: >=2 tied at lowest → bonus round (no elimination)
    if (tie_count >= 2) {
        printf("[Round1] %d players tied at lowest (%d), triggering bonus round\n", tie_count, lowest);
        
        // Collect tied players
        int32_t tied_players[MAX_MATCH_PLAYERS];
        int tied_count = 0;
        for (int i = 0; i < count && tied_count < MAX_MATCH_PLAYERS; i++) {
            if (scores[i].score == lowest) {
                tied_players[tied_count++] = scores[i].account_id;
            }
        }
        
        // Trigger bonus round
        if (match && tied_count >= 2) {
            check_and_trigger_bonus(match->runtime_match_id, 1);
            return true;  // Bonus triggered - don't advance to next round
        }
        return false;
    }

    // STEP 5: Eliminate lowest scorer
    MatchPlayerState *elim_player = get_match_player(scores[0].account_id);
    if (elim_player) {
        eliminate_player(elim_player, "LOWEST_SCORE");
    }
    
    return false;  // Normal elimination, continue to next round
}

//==============================================================================
// HELPER: Build round end result
// Format matches Frontend expectations:
// - players[] instead of rankings[]
// - id instead of account_id  
// - name field included
// - finished_count, player_count for progress
//==============================================================================

static char* build_round_end_json(void) {
    MatchState *match = get_match();
    if (!match) return NULL;
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddTrueToObject(obj, "success");
    cJSON_AddNumberToObject(obj, "match_id", g_r1.match_id);
    cJSON_AddNumberToObject(obj, "round", 1);
    
    int connected = count_connected();
int active = count_active_players(g_r1.match_id);
bool can_continue = (active >= 2);    
    // Frontend expects these names
    cJSON_AddNumberToObject(obj, "connected_count", connected);
    cJSON_AddNumberToObject(obj, "finished_count", g_r1.player_count);
    cJSON_AddNumberToObject(obj, "player_count", g_r1.player_count);
    
 if (!can_continue) {
    cJSON_AddStringToObject(obj, "status_message", "Game ended");
} else {
    cJSON_AddStringToObject(obj, "status_message", "Round completed");
}
    
    // Check for bonus round
    ScoreEntry scores[MAX_MATCH_PLAYERS];
    int count = 0;
    for (int i = 0; i < g_r1.player_count; i++) {
        MatchPlayerState *mp = get_match_player(g_r1.players[i].account_id);
        if (mp) {
            scores[count].account_id = mp->account_id;
            scores[count].score = mp->score;
            count++;
      }
    }
    
    // Sort ascending to find lowest
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (scores[j].score > scores[j+1].score) {
                ScoreEntry tmp = scores[j];
                scores[j] = scores[j+1];
                scores[j+1] = tmp;
            }
        }
    }
    
   bool need_bonus = false;
if (can_continue && match->mode == MODE_ELIMINATION && count >= 2) {
    int lowest = scores[0].score;
    int tie = 0;
    for (int i = 0; i < count; i++) {
        if (scores[i].score == lowest) tie++;
    }
    if (tie >= 2) need_bonus = true;
}
    cJSON_AddBoolToObject(obj, "need_bonus_round", need_bonus);
    
    // Add next round info (match was already retrieved at start of function)
    int next_round = match->current_round_idx + 1; // 1-based for frontend
    if (next_round > match->round_count) {
        next_round = 0; // No more rounds
    }
    cJSON_AddNumberToObject(obj, "next_round", next_round);
    cJSON_AddNumberToObject(obj, "current_round", 1);
    
    // Sort descending for display (highest first)
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (scores[j].score < scores[j+1].score) {
                ScoreEntry tmp = scores[j];
                scores[j] = scores[j+1];
                scores[j+1] = tmp;
            }
        }
    }
    
    // Build players array (Frontend expects "players" not "rankings")
    cJSON *players = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        MatchPlayerState *mp = get_match_player(scores[i].account_id);
        if (!mp) continue;
        
        cJSON *p = cJSON_CreateObject();
        // Frontend expects "id" not "account_id"
        cJSON_AddNumberToObject(p, "id", mp->account_id);
        cJSON_AddNumberToObject(p, "rank", i + 1);
        cJSON_AddNumberToObject(p, "score", mp->score);
        cJSON_AddBoolToObject(p, "connected", is_connected(mp->account_id));
        cJSON_AddBoolToObject(p, "eliminated", mp->eliminated != 0);
        
        // Add player name (format: "Player{id}")
        // Add player name
        cJSON_AddStringToObject(p, "name", mp->name);
        
        cJSON_AddItemToArray(players, p);
    }
    cJSON_AddItemToObject(obj, "players", players);
    
    // Also add rankings for backwards compatibility
    cJSON_AddItemToObject(obj, "rankings", cJSON_Duplicate(players, 1));
    
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    return json;
}

//==============================================================================
// HELPER: Send current question to all, move round to next question
//==============================================================================

static void broadcast_current_question(MessageHeader *req) {
    RoundState *round = get_round();
    if (!round) return;
    
    int q_idx = round->current_question_idx;
    printf("[Round1] Broadcasting question %d/%d\n", q_idx + 1, round->question_count);
    
    // Reset answered flags for new question
    reset_answered_flags();
    
    // Update QuestionState status
    if (q_idx < round->question_count) {
        round->questions[q_idx].status = QUESTION_ACTIVE;
    }
    
    char *json = build_question_json(q_idx);
    if (json) {
        printf("[Round1] Question data: %s\n", json);
        broadcast_json(req, OP_S2C_ROUND1_QUESTION, json);
        free(json);
        
        // ⭐ Start timeout timer for this question
        start_question_timer(q_idx);
    } else {
        printf("[Round1] ERROR: Failed to build question JSON for idx=%d\n", q_idx);
    }
}
void advance_to_next_question(MessageHeader *req) {
    stop_question_timer();

    RoundState *round = get_round();
    if (!round) return;

    int curr = round->current_question_idx;
    if (curr < round->question_count) {
        round->questions[curr].status = QUESTION_ENDED;
    }

    round->current_question_idx++;

    // =================================================
    // CHƯA HẾT ROUND → NEXT QUESTION
    // =================================================
    if (round->current_question_idx < round->question_count) {
        broadcast_current_question(req);
        return;
    }

    // =================================================
    // END ROUND
    // =================================================
    printf("[Round1] All questions completed\n");
    round->status = ROUND_ENDED;
    round->ended_at = time(NULL);
    g_r1.is_active = false;

    printf("[Round1] About to call r1_finalize_round...\n");
    bool bonus_triggered = false;
    bool match_ended = r1_finalize_round(req, &bonus_triggered);
    
    printf("[Round1] After finalize: match_ended=%d, bonus_triggered=%d\n", match_ended, bonus_triggered);

    // =================================================
    // Nếu match CHƯA END → broadcast end-round summary
    // =================================================
    if (!match_ended) {
        printf("[Round1] Building round end JSON...\n");
        char *json = build_round_end_json();
        if (json) {
            // Add bonus_triggered flag if bonus was triggered
            if (bonus_triggered) {
                printf("[Round1] Bonus round triggered - adding bonus_triggered flag to summary\n");
                
                // Parse JSON, add bonus_triggered flag, then re-serialize
                cJSON *obj = cJSON_Parse(json);
                if (obj) {
                    cJSON_AddBoolToObject(obj, "bonus_triggered", true);
                    free(json);
                    json = cJSON_PrintUnformatted(obj);
                    cJSON_Delete(obj);
                    printf("[Round1] Updated JSON with bonus_triggered flag\n");
                }
            }
            
            printf("[Round1] Broadcasting OP_S2C_ROUND1_ALL_FINISHED: %s\n", json);
            broadcast_json(req, OP_S2C_ROUND1_ALL_FINISHED, json);
            free(json);
        } else {
            printf("[Round1] ERROR: Failed to build round end JSON!\n");
        }
    } else {
        printf("[Round1] Match ended, not sending ALL_FINISHED\n");
    }
}


//==============================================================================
// HANDLER: Player Ready (OP_C2S_ROUND1_PLAYER_READY)
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

    // Get account from session
    UserSession *session = session_get_by_socket(fd);
    int32_t account_id = session ? session->account_id : (int32_t)player_id;
    
    printf("[Round1] Player ready: match=%u account=%d\n", match_id, account_id);

    // Verify match exists
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

    // Block eliminated players (includes disconnect in both modes)
    if (mp->eliminated) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Player eliminated\"}");
        return;
    }
    
    // Initialize context if new match
    if (match_id != g_r1.match_id) {
        reset_context();
        g_r1.match_id = match_id;
        g_r1.round_index = match->current_round_idx;
    }
    
    // Add to local tracking
    R1_PlayerAnswer *pa = add_player(account_id);
    if (!pa) {
        send_json(fd, req, ERR_ROOM_FULL, "{\"success\":false,\"error\":\"Round full\"}");
        return;
    }
    
    // Check for reconnection during active round
    RoundState *round = get_round();
    if (round && round->status == ROUND_PLAYING) {
        // Send current question
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddTrueToObject(obj, "success");
        cJSON_AddTrueToObject(obj, "reconnected");
        cJSON_AddNumberToObject(obj, "current_score", mp->score);
        cJSON_AddNumberToObject(obj, "current_question", round->current_question_idx);
        
        // Add full player list for leaderboard
        cJSON *players = cJSON_CreateArray();
        for (int i = 0; i < g_r1.player_count; i++) {
            cJSON *p = cJSON_CreateObject();
            cJSON_AddNumberToObject(p, "account_id", g_r1.players[i].account_id);
            cJSON_AddBoolToObject(p, "ready", g_r1.players[i].ready); // or connected status
            
            MatchPlayerState *mp_state = get_match_player(g_r1.players[i].account_id);
            if (mp_state) {
                cJSON_AddStringToObject(p, "name", mp_state->name);
                cJSON_AddNumberToObject(p, "score", mp_state->score);
            }
            cJSON_AddItemToArray(players, p);
        }
        cJSON_AddItemToObject(obj, "players", players);

        char *json = cJSON_PrintUnformatted(obj);
        cJSON_Delete(obj);
        send_json(fd, req, OP_S2C_ROUND1_READY_STATUS, json);
        free(json);
      
        // Also send current question
        char *q_json = build_question_json(round->current_question_idx);
        if (q_json) {
            send_json(fd, req, OP_S2C_ROUND1_QUESTION, q_json);
            free(q_json);
        }
        return;
    }
    
    // Mark ready
    if (!pa->ready) {
        pa->ready = true;
        g_r1.ready_count++;
    }
    
    // Broadcast ready status
    cJSON *status = cJSON_CreateObject();
    cJSON_AddTrueToObject(status, "success");
    cJSON_AddNumberToObject(status, "ready_count", g_r1.ready_count);
    cJSON_AddNumberToObject(status, "player_count", g_r1.player_count);
    cJSON_AddNumberToObject(status, "required_players", match->player_count);
    
    cJSON *players = cJSON_CreateArray();
    for (int i = 0; i < g_r1.player_count; i++) {
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "account_id", g_r1.players[i].account_id);
        cJSON_AddBoolToObject(p, "ready", g_r1.players[i].ready);
        
        MatchPlayerState *mp_state = get_match_player(g_r1.players[i].account_id);
        if (mp_state) {
            cJSON_AddStringToObject(p, "name", mp_state->name);
        }
        
        cJSON_AddItemToArray(players, p);
    }
    cJSON_AddItemToObject(status, "players", players);
    
    char *json = cJSON_PrintUnformatted(status);
    cJSON_Delete(status);
    broadcast_json(req, OP_S2C_ROUND1_READY_STATUS, json);
    free(json);

    // All ready → start round
//     // ⭐ HARDCODE FOR TESTING: Start with 1+ player ready
//     printf("[Round1] Check start: ready=%d, match_players=%d, round_status=%d\n",
//         g_r1.ready_count, match->player_count, round ? round->status : -1);
//  // ⭐ HARDCODE FOR TESTING: Require 2 players ready
//  if (g_r1.ready_count >= 2 && round && round->status == ROUND_PENDING) {
    // Calculate expected players (non-eliminated)
    int expected = 0;
    for (int i = 0; i < match->player_count; i++) {
        if (!match->players[i].eliminated) expected++;
    }
    
    printf("[Round1] Check start: ready=%d, expected=%d, match_players=%d, round_status=%d\n",
           g_r1.ready_count, expected, match->player_count, round ? round->status : -1);
    
    // Start when all non-eliminated players are ready
    if (g_r1.ready_count >= expected && expected > 0 && round && round->status == ROUND_PENDING) {
        printf("[Round1] All ready, starting round\n");
        
        // Update RoundState
        round->status = ROUND_PLAYING;
        round->started_at = time(NULL);
        round->current_question_idx = 0;
        g_r1.is_active = true;
    
        // Build start message
        cJSON *start = cJSON_CreateObject();
        cJSON_AddTrueToObject(start, "success");
        cJSON_AddNumberToObject(start, "match_id", match_id);
        cJSON_AddNumberToObject(start, "total_questions", round->question_count);
        cJSON_AddNumberToObject(start, "time_per_question_ms", TIME_PER_QUESTION);
        cJSON_AddNumberToObject(start, "player_count", g_r1.player_count);
        
        char *start_json = cJSON_PrintUnformatted(start);
        cJSON_Delete(start);
        broadcast_json(req, OP_S2C_ROUND1_ALL_READY, start_json);
        free(start_json);
        
        // Send first question
        broadcast_current_question(req);
  }
}

//==============================================================================
// HANDLER: Get Question (OP_C2S_ROUND1_GET_QUESTION)
//==============================================================================

static void handle_get_question(int fd, MessageHeader *req, const char *payload) {
    if (req->length < 8) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid payload\"}");
    return;
  }

    uint32_t match_id, q_idx;
    memcpy(&match_id, payload, 4);
    memcpy(&q_idx, payload + 4, 4);
    match_id = ntohl(match_id);
    
    if (match_id != g_r1.match_id) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid match\"}");
        return;
  }
  
    RoundState *round = get_round();
    if (!round || round->status != ROUND_PLAYING) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Round not active\"}");
    return;
  }

    // Check if player is eliminated
    // Check if player is eliminated (includes disconnect in both modes)
    UserSession *session = session_get_by_socket(fd);
    if (session) {
        MatchPlayerState *mp = get_match_player(session->account_id);
        if (mp && mp->eliminated) {
            send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Player eliminated\"}");
    return;
  }
    }
    
    // Send current question (use RoundState.current_question_idx)
    char *json = build_question_json(round->current_question_idx);
    if (!json) {
        send_json(fd, req, ERR_SERVER_ERROR, "{\"success\":false,\"error\":\"Question not found\"}");
    return;
  }

    send_json(fd, req, OP_S2C_ROUND1_QUESTION, json);
    free(json);
}

//==============================================================================
// HANDLER: Submit Answer (OP_C2S_ROUND1_ANSWER)
// Updates: MatchPlayerState.score, MatchPlayerState.score_delta
// Uses: RoundState.current_question_idx, QuestionState.answered_count
//==============================================================================
  
// ⭐ Option 1: Network buffer time (2 seconds) to handle network latency
#define NETWORK_BUFFER_MS 2000

static void handle_submit_answer(int fd, MessageHeader *req, const char *payload) {
  if (req->length < 13) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid payload\"}");
    return;
  }

    uint32_t match_id, q_idx, time_ms;
    uint8_t choice;

    memcpy(&match_id, payload, 4);
    memcpy(&q_idx, payload + 4, 4);
    choice = (uint8_t)payload[8];
    memcpy(&time_ms, payload + 9, 4);
    
    match_id = ntohl(match_id);
    q_idx = ntohl(q_idx);
    time_ms = ntohl(time_ms);
    
    printf("[Round1] Answer: q=%u choice=%u time=%ums\n", q_idx, choice, time_ms);
    
    if (match_id != g_r1.match_id) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid match\"}");
        return;
    }
    
    RoundState *round = get_round();
    if (!round || round->status != ROUND_PLAYING) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Round not active\"}");
        return;
    }
    
    // ⭐ Option 1: Server-side time validation (anti-cheat)
    // Calculate actual elapsed time from server's perspective
    time_t current_time = time(NULL);
    uint32_t actual_time_ms = (uint32_t)((current_time - g_r1.question_start_time) * 1000);
    
    // Check if answer arrived too late (beyond time limit + buffer)
    if (actual_time_ms > TIME_PER_QUESTION + NETWORK_BUFFER_MS) {
        printf("[Round1] Answer too late: actual=%ums limit=%d buffer=%d\n",
               actual_time_ms, TIME_PER_QUESTION, NETWORK_BUFFER_MS);
        send_json(fd, req, ERR_BAD_REQUEST, 
                  "{\"success\":false,\"error\":\"Answer received too late\"}");
        return;
    }
    
    // Get player session
    UserSession *session = session_get_by_socket(fd);
    if (!session) {
        send_json(fd, req, ERR_NOT_LOGGED_IN, "{\"success\":false,\"error\":\"Not logged in\"}");
        return;
    }
    
    // Get MatchPlayerState
    MatchPlayerState *mp = get_match_player(session->account_id);
    if (!mp) {
        send_json(fd, req, ERR_SERVER_ERROR, "{\"success\":false,\"error\":\"Player not in match\"}");
        return;
  }

    // Block eliminated players (includes disconnect in both modes)
    if (mp->eliminated) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Player eliminated\"}");
        return;
    }
    
    // Get local tracking
    R1_PlayerAnswer *pa = find_player(session->account_id);
    if (!pa) {
        send_json(fd, req, ERR_SERVER_ERROR, "{\"success\":false,\"error\":\"Player not in round\"}");
        return;
    }
    
    // Verify answering current question (use RoundState.current_question_idx)
    if ((int)q_idx != round->current_question_idx) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Wrong question\"}");
        return;
    }
    
    // Check if already answered (with graceful handling for timeout race condition)
    if (pa->answered_current) {
        // ⭐ Option 2: Graceful handling for network latency
        // If answer arrived within buffer time after timeout, it was a race condition
        if (actual_time_ms <= TIME_PER_QUESTION + NETWORK_BUFFER_MS) {
            // Answer arrived late but within buffer - this is a race condition
            // The timeout already marked player as answered, so just acknowledge
            printf("[Round1] Answer arrived after timeout but within buffer - race condition\n");
        }
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Already answered\"}");
        return;
    }
    
    // Mark as answered (local tracking)
    pa->answered_current = true;
    
    // ⭐ Option 1: Use server-calculated time for scoring (anti-cheat)
    // Clamp to TIME_PER_QUESTION if slightly over due to network latency
    uint32_t scoring_time_ms = time_ms;
    if (actual_time_ms > TIME_PER_QUESTION) {
        scoring_time_ms = TIME_PER_QUESTION;  // Cap at max time
    }
    
    // Calculate score using correct_index from question data
    int correct_idx = get_correct_index((int)q_idx);
    bool correct = (choice <= 3) && ((int)choice == correct_idx);
    int delta = calc_score(correct, scoring_time_ms);
    
    // UPDATE MatchPlayerState.score
    mp->score += delta;
    mp->score_delta = delta;
    
    // UPDATE QuestionState (in RoundState)
    if ((int)q_idx < round->question_count) {
        round->questions[q_idx].answered_count++;
        if (correct) {
            round->questions[q_idx].correct_count++;
        }
  }

    printf("[Round1] Player %d: correct=%d delta=%d total=%d\n",
           mp->account_id, correct, delta, mp->score);
    
    // ⭐ SAVE ANSWER TO DATABASE using db_match_answer_insert
    if (mp->match_player_id > 0 && round->questions[q_idx].question_id > 0) {
        // Build answer JSON string
        cJSON *ans_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(ans_obj, "answer", choice);
        cJSON_AddBoolToObject(ans_obj, "is_correct", correct);
        cJSON_AddNumberToObject(ans_obj, "time_ms", time_ms);
        char *ans_json = cJSON_PrintUnformatted(ans_obj);
        cJSON_Delete(ans_obj);

        // Use db_match_answer_insert from match_repo
        db_error_t db_err = db_match_answer_insert(
            round->questions[q_idx].question_id,
            mp->match_player_id,
            ans_json,
            delta,
            1  // action_idx = 1
        );
        
        if (db_err == DB_SUCCESS) {
            printf("[Round1] Saved answer to DB: q=%d p=%d delta=%d\n",
                   round->questions[q_idx].question_id, mp->match_player_id, delta);
        } else {
            printf("[Round1] Failed to save answer: %d\n", db_err);
        }
        
        if (ans_json) free(ans_json);
    }

    // Build response
    cJSON *result = cJSON_CreateObject();
    cJSON_AddTrueToObject(result, "success");
    cJSON_AddNumberToObject(result, "question_idx", q_idx);
    cJSON_AddBoolToObject(result, "is_correct", correct);
    cJSON_AddNumberToObject(result, "score", delta);           // Frontend expects "score"
    cJSON_AddNumberToObject(result, "score_delta", delta);     // Also include score_delta for new clients
    cJSON_AddNumberToObject(result, "current_score", mp->score);
    cJSON_AddNumberToObject(result, "correct_index", correct_idx);
    
    int answered = count_answered();
    int connected = count_connected();
    cJSON_AddNumberToObject(result, "answered_count", answered);
    cJSON_AddNumberToObject(result, "player_count", connected);
    
    bool all_answered = (answered >= connected);
    cJSON_AddBoolToObject(result, "all_answered", all_answered);
        
    char *json = cJSON_PrintUnformatted(result);
    cJSON_Delete(result);
    printf("[Round1] Result JSON: %s\n", json);
    send_json(fd, req, OP_S2C_ROUND1_RESULT, json);
    free(json);
        
    // If all answered, advance to next question
    if (all_answered) {
        printf("[Round1] All %d players answered question %d\n", connected, q_idx);
        advance_to_next_question(req);
    }
}

//==============================================================================
// HANDLER: End Round (OP_C2S_ROUND1_END / OP_C2S_ROUND1_FINISHED)
//==============================================================================

static void handle_end_round(int fd, MessageHeader *req, const char *payload) {
    (void)payload;
    
    UserSession *session = session_get_by_socket(fd);
    MatchPlayerState *mp = session ? get_match_player(session->account_id) : NULL;
    
    // Send waiting status
    cJSON *wait = cJSON_CreateObject();
    cJSON_AddTrueToObject(wait, "success");
    cJSON_AddTrueToObject(wait, "waiting");
    if (mp) cJSON_AddNumberToObject(wait, "your_score", mp->score);
    
    char *json = cJSON_PrintUnformatted(wait);
    cJSON_Delete(wait);
    send_json(fd, req, OP_S2C_ROUND1_WAITING, json);
    free(json);
}

//==============================================================================
// HANDLER: Legacy Ready (OP_C2S_ROUND1_READY)
//==============================================================================

static void handle_legacy_ready(int fd, MessageHeader *req, const char *payload) {
    if (req->length < 8) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid payload\"}");
    return;
  }

    uint32_t room_id, match_id;
    memcpy(&room_id, payload, 4);
    memcpy(&match_id, payload + 4, 4);
    room_id = ntohl(room_id);
  match_id = ntohl(match_id);
  
    MatchState *match = match_get_by_id(match_id);
    if (!match) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Match not found\"}");
        return;
    }
    
    RoundState *round = (match->round_count > 0) ? &match->rounds[0] : NULL;
    if (!round || round->question_count == 0) {
        send_json(fd, req, ERR_SERVER_ERROR, "{\"success\":false,\"error\":\"No questions\"}");
        return;
    }
    
    reset_context();
    g_r1.match_id = match_id;
    g_r1.round_index = 0;
    g_r1.is_active = true;
    
    round->status = ROUND_PLAYING;
    round->started_at = time(NULL);
    round->current_question_idx = 0;
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddTrueToObject(resp, "success");
    cJSON_AddNumberToObject(resp, "match_id", match_id);
    cJSON_AddNumberToObject(resp, "room_id", room_id);
    cJSON_AddNumberToObject(resp, "total_questions", round->question_count);
    cJSON_AddNumberToObject(resp, "time_per_question", TIME_PER_QUESTION / 1000);
    cJSON_AddNumberToObject(resp, "round", 1);
    
    char *json = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    send_json(fd, req, OP_S2C_ROUND1_START, json);
    free(json);
}

//==============================================================================
// DISCONNECT HANDLER
//==============================================================================
void handle_round1_disconnect(int client_fd) {
    UserSession *session = session_get_by_socket(client_fd);
    if (!session) return;

    int32_t account_id = session->account_id;
    R1_PlayerAnswer *pa = find_player(account_id);
    if (!pa) return;

    printf("[Round1] Player %d disconnected\n", account_id);

    MatchPlayerState *mp = get_match_player(account_id);
    if (mp) mp->connected = 0;

    int connected = count_connected();
    int active = count_active_players(g_r1.match_id);

    cJSON *ntf = cJSON_CreateObject();
    cJSON_AddNumberToObject(ntf, "player_id", account_id);
    cJSON_AddStringToObject(ntf, "event", "disconnect");
    cJSON_AddNumberToObject(ntf, "connected_count", connected);
    cJSON_AddNumberToObject(ntf, "active_count", active);

    if (active < 2) {
        cJSON_AddTrueToObject(ntf, "game_ended");
        cJSON_AddStringToObject(ntf, "reason", "ACTIVE_PLAYERS_LT_2");
    }

    char *json = cJSON_PrintUnformatted(ntf);
    cJSON_Delete(ntf);

    MessageHeader hdr = {0};
    hdr.magic = htons(MAGIC_NUMBER);
    hdr.version = PROTOCOL_VERSION;
    hdr.command = htons(NTF_PLAYER_LEFT);
    hdr.length = htonl((uint32_t)strlen(json));

    for (int i = 0; i < g_r1.player_count; i++) {
        if (g_r1.players[i].account_id != account_id) {
            int fd = get_socket(g_r1.players[i].account_id);
            if (fd > 0) {
                send(fd, &hdr, sizeof(hdr), 0);
                send(fd, json, strlen(json), 0);
            }
        }
    }
    free(json);

    RoundState *round = get_round();
    if (round && round->status == ROUND_PLAYING) {
        if (active < 2) {
            MessageHeader dummy = {0};
            end_game_now(&dummy, "ACTIVE_PLAYERS_LT_2_DURING_ROUND");
            return;
        }

        int answered = count_answered();
        if (answered >= connected && connected > 0) {
            MessageHeader dummy = {0};
            advance_to_next_question(&dummy);
        }
    }
}

//==============================================================================
// MAIN DISPATCHER
//==============================================================================

void handle_round1(int client_fd, MessageHeader *req_header, const char *payload) {
  printf("[Round1] cmd=0x%04X len=%u\n", req_header->command, req_header->length);

  switch (req_header->command) {
    case OP_C2S_ROUND1_READY:
            handle_legacy_ready(client_fd, req_header, payload);
      break;
    case OP_C2S_ROUND1_PLAYER_READY:
      handle_player_ready(client_fd, req_header, payload);
      break;
    case OP_C2S_ROUND1_GET_QUESTION:
      handle_get_question(client_fd, req_header, payload);
      break;
    case OP_C2S_ROUND1_ANSWER:
      handle_submit_answer(client_fd, req_header, payload);
      break;
    case OP_C2S_ROUND1_END:
    case OP_C2S_ROUND1_FINISHED:
      handle_end_round(client_fd, req_header, payload);
      break;
    default:
            printf("[Round1] Unknown cmd: 0x%04X\n", req_header->command);
      break;
  }
}
