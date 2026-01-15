/**
 * round2_handler.c - Round 2: The Bid
 * Players bid on product prices, closest without going over wins.
 * 
 * STATE USAGE:
 * - Score: MatchPlayerState.score - READ/UPDATE via get_match_player()
 * - Eliminated: MatchPlayerState.eliminated - READ
 * - Connected: UserSession.state - READ via session_get_by_account()
 * - Current product: RoundState.current_question_idx - READ/UPDATE
 * - Product data: RoundState.question_data[] - READ only
 * 
 * LOCAL TRACKING (Round-specific):
 * - bid_value: Player's bid for CURRENT product
 * - has_bid: Has player submitted bid for current product?
 * - ready: Is player ready to start the round?
 * 
 * Flow:
 * 1. All players ready → Round starts
 * 2. Server sends current product to all players
 * 3. Players submit bids → Server stores bids
 * 4. When all bid → Calculate scores, send results
 * 5. After 5 products → Check elimination, show final results
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

#include "handlers/round2_handler.h"
#include "handlers/session_manager.h"
#include "handlers/match_manager.h"
#include "handlers/start_game_handler.h"
#include "handlers/bonus_handler.h"
#include "db/core/db_client.h"
#include "db/repo/match_repo.h"         // For db_match_event_insert, db_match_answer_insert
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include <cjson/cJSON.h>

// For be64toh on macOS/Linux
#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define be64toh(x) OSSwapBigToHostInt64(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#else
#include <endian.h>
#endif

//==============================================================================
// CONSTANTS
//==============================================================================
// ⭐ HARDCODE FOR TESTING: Reduced time (10 seconds)
#define TIME_PER_PRODUCT    15000  // 20 seconds for testing
#define SCORE_CLOSEST       100    // Score for closest bid without going over
#define SCORE_OVERBID       50     // Score if all overbid (closest overbid wins)
#define MAX_PRODUCTS        5      // 5 products per round

//==============================================================================
// ROUND 2 EXECUTION CONTEXT
//==============================================================================

typedef struct {
    int32_t account_id;
    int64_t bid_value;      // Player's bid (price guess), -1 = not bid yet
    bool    has_bid;        // Has submitted bid for current product
    bool    ready;          // Ready for round to start
} R2_PlayerBid;

typedef struct {
    uint32_t match_id;
    int      round_index;
    bool     is_active;
    
    R2_PlayerBid players[MAX_MATCH_PLAYERS];
    int      player_count;
    int      ready_count;
    
    // Timer for product timeout
    time_t   product_start_time;
    int      current_timer_idx;
    pthread_t timer_thread;
    bool     timer_running;
} R2_Context;

static R2_Context g_r2 = {0};
static pthread_mutex_t g_r2_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations
static void process_turn_results(MessageHeader *req);
static void advance_to_next_product(MessageHeader *req);
static void broadcast_current_product(MessageHeader *req);

//==============================================================================
// HELPER: Reset context
//==============================================================================

static void reset_context(void) {
    pthread_mutex_lock(&g_r2_mutex);
    g_r2.timer_running = false;
    pthread_mutex_unlock(&g_r2_mutex);
    
    memset(&g_r2, 0, sizeof(g_r2));
    printf("[Round2] Context reset\n");
}

//==============================================================================
// HELPER: Get states from other PICs
//==============================================================================

static MatchState* get_match(void) {
    if (g_r2.match_id == 0) return NULL;
    return match_get_by_id(g_r2.match_id);
}

static RoundState* get_round(void) {
    MatchState *match = get_match();
    if (!match) return NULL;
    if (g_r2.round_index < 0 || g_r2.round_index >= match->round_count) {
        return NULL;
    }
    return &match->rounds[g_r2.round_index];
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
    if (!session || session->state == SESSION_PLAYING_DISCONNECTED) {
        return -1;
    }
    return session->socket_fd;
}

//==============================================================================
// HELPER: Local bid tracking
//==============================================================================

static R2_PlayerBid* find_player(int32_t account_id) {
    for (int i = 0; i < g_r2.player_count; i++) {
        if (g_r2.players[i].account_id == account_id) {
            return &g_r2.players[i];
        }
    }
    return NULL;
}

static R2_PlayerBid* add_player(int32_t account_id) {
    R2_PlayerBid *existing = find_player(account_id);
    if (existing) return existing;
    
    if (g_r2.player_count >= MAX_MATCH_PLAYERS) return NULL;
    
    R2_PlayerBid *p = &g_r2.players[g_r2.player_count++];
    p->account_id = account_id;
    p->bid_value = -1;
    p->has_bid = false;
    p->ready = false;
    return p;
}

static void reset_bids(void) {
    for (int i = 0; i < g_r2.player_count; i++) {
        g_r2.players[i].bid_value = -1;
        g_r2.players[i].has_bid = false;
    }
}

static int count_bid(void) {
    int count = 0;
    for (int i = 0; i < g_r2.player_count; i++) {
        if (g_r2.players[i].has_bid) {
            count++;
        }
    }
    return count;
}

static int count_connected(void) {
    int count = 0;
    for (int i = 0; i < g_r2.player_count; i++) {
        if (is_connected(g_r2.players[i].account_id)) {
            count++;
        }
    }
    return count;
}

static int count_disconnected(void) {
    return g_r2.player_count - count_connected();
}

//==============================================================================
// HELPER: Get correct price from product data
//==============================================================================

static int64_t get_correct_price(int product_idx) {
    RoundState *round = get_round();
    if (!round || product_idx < 0 || product_idx >= round->question_count) return -1;
    
    MatchQuestion *mq = &round->question_data[product_idx];
    if (!mq->json_data) return -1;
    
    cJSON *root = cJSON_Parse(mq->json_data);
    if (!root) return -1;
    
    // Product data may be nested inside "data" field from DB
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!data) {
        data = root;  // Fallback: data at root level
    }
    
    int64_t price = -1;
    
    // Look for "correct_answer" field (number)
    cJSON *cans = cJSON_GetObjectItem(data, "correct_answer");
    if (cans && cJSON_IsNumber(cans)) {
        price = (int64_t)cans->valuedouble;
        printf("[Round2] Product %d: correct_price = %lld\n", product_idx, (long long)price);
    } else {
        // Try "answer" field
        cJSON *ans = cJSON_GetObjectItem(data, "answer");
        if (ans && cJSON_IsNumber(ans)) {
            price = (int64_t)ans->valuedouble;
            printf("[Round2] Product %d: answer = %lld\n", product_idx, (long long)price);
        } else {
            // Try "price" field
            cJSON *prc = cJSON_GetObjectItem(data, "price");
            if (prc && cJSON_IsNumber(prc)) {
                price = (int64_t)prc->valuedouble;
                printf("[Round2] Product %d: price = %lld\n", product_idx, (long long)price);
            }
        }
    }
    
    cJSON_Delete(root);
    return price;
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
    for (int i = 0; i < g_r2.player_count; i++) {
        int fd = get_socket(g_r2.players[i].account_id);
        if (fd > 0) {
            forward_response(fd, req, cmd, json, (uint32_t)strlen(json));
        }
    }
}

//==============================================================================
// HELPER: Build product payload
//==============================================================================

static char* build_product_json(int product_idx) {
    RoundState *round = get_round();
    if (!round || product_idx < 0 || product_idx >= round->question_count) return NULL;
    
    MatchQuestion *mq = &round->question_data[product_idx];
    if (!mq->json_data) return NULL;
    
    cJSON *root = cJSON_Parse(mq->json_data);
    if (!root) return NULL;
    
    // Product data may be nested inside "data" field from DB
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!data) {
        data = root;  // Fallback: data at root level
    }
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddTrueToObject(obj, "success");
    cJSON_AddNumberToObject(obj, "product_idx", product_idx);
    cJSON_AddNumberToObject(obj, "total_products", round->question_count);
    cJSON_AddNumberToObject(obj, "time_limit_ms", TIME_PER_PRODUCT);
    
    // ⭐ Option 3: Send server timestamp for client-side time sync
    cJSON_AddNumberToObject(obj, "start_timestamp", (double)g_r2.product_start_time);
    
    // Copy question text - try multiple field names
    cJSON *text = cJSON_GetObjectItem(data, "question");
    if (!text) text = cJSON_GetObjectItem(data, "content");
    if (!text) text = cJSON_GetObjectItem(data, "product_name");
    if (text && cJSON_IsString(text)) {
        cJSON_AddStringToObject(obj, "question", text->valuestring);
    }
    
    // Copy product image
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
// SCORING: Calculate scores for all players
// Returns winner_id (account_id of winner, 0 if no winner)
//==============================================================================

typedef struct {
    int32_t account_id;
    int64_t bid;
    int64_t diff;       // difference from correct price
    bool    over;       // true if bid > correct_price
    int     score;      // calculated score
} BidResult;

static void calculate_turn_scores(int64_t correct_price, BidResult *results, int count) {
    if (count == 0 || correct_price <= 0) return;
    
    // Calculate difference for each player
    int under_count = 0;
    int over_count = 0;
    int64_t closest_under = INT64_MAX;
    int64_t closest_over = INT64_MAX;
    
    for (int i = 0; i < count; i++) {
        results[i].diff = llabs(results[i].bid - correct_price);
        results[i].over = (results[i].bid > correct_price);
        results[i].score = 0;
        
        if (results[i].bid < 0) {
            // No bid = treated as over by huge margin
            results[i].over = true;
            results[i].diff = INT64_MAX;
        } else if (!results[i].over) {
            under_count++;
            if (results[i].diff < closest_under) {
                closest_under = results[i].diff;
            }
        } else {
            over_count++;
            if (results[i].diff < closest_over) {
                closest_over = results[i].diff;
            }
        }
        
        printf("[Round2] Player %d: bid=%lld diff=%lld over=%d\n", 
               results[i].account_id, (long long)results[i].bid, 
               (long long)results[i].diff, results[i].over);
    }
    
    // Scoring logic
    if (under_count > 0) {
        // Case 1: At least one bid <= correct price
        // Winner = closest under (100 points)
        int winner_count = 0;
        for (int i = 0; i < count; i++) {
            if (!results[i].over && results[i].diff == closest_under) {
                results[i].score = SCORE_CLOSEST;
                winner_count++;
                printf("[Round2] Winner (under): Player %d +%d\n", 
                       results[i].account_id, SCORE_CLOSEST);
            }
        }
        
        // Handle tie - split points if multiple winners
        if (winner_count > 1) {
            int split = SCORE_CLOSEST / winner_count;
            for (int i = 0; i < count; i++) {
                if (results[i].score > 0) {
                    results[i].score = split;
                }
            }
        }
    } else {
        // Case 2: ALL players overbid
        // Winner = closest over (50 points)
        int winner_count = 0;
        for (int i = 0; i < count; i++) {
            if (results[i].diff == closest_over && results[i].diff < INT64_MAX) {
                results[i].score = SCORE_OVERBID;
                winner_count++;
                printf("[Round2] Winner (overbid): Player %d +%d\n", 
                       results[i].account_id, SCORE_OVERBID);
            }
        }
        
        // Handle tie
        if (winner_count > 1) {
            int split = SCORE_OVERBID / winner_count;
            for (int i = 0; i < count; i++) {
                if (results[i].score > 0) {
                    results[i].score = split;
                }
            }
        }
    }
}

//==============================================================================
// TIMER: Product timeout handler
//==============================================================================

static void* product_timer_thread(void *arg) {
    (void)arg;
    
    int target_idx = g_r2.current_timer_idx;
    uint32_t target_match = g_r2.match_id;
    
    printf("[Round2-Timer] Started for product %d (timeout: %dms)\n", 
           target_idx, TIME_PER_PRODUCT);
    
    int sleep_time = TIME_PER_PRODUCT / 1000;
    for (int i = 0; i < sleep_time; i++) {
        sleep(1);
        
        pthread_mutex_lock(&g_r2_mutex);
        bool still_running = g_r2.timer_running;
        bool same_product = (g_r2.current_timer_idx == target_idx && 
                            g_r2.match_id == target_match);
        pthread_mutex_unlock(&g_r2_mutex);
        
        if (!still_running || !same_product) {
            printf("[Round2-Timer] Cancelled (product already advanced)\n");
            return NULL;
        }
    }
    
    pthread_mutex_lock(&g_r2_mutex);
    bool should_advance = g_r2.timer_running && 
                         g_r2.current_timer_idx == target_idx &&
                         g_r2.match_id == target_match &&
                         g_r2.is_active;
    pthread_mutex_unlock(&g_r2_mutex);
    
    if (should_advance) {
        printf("[Round2-Timer] ⏰ TIMEOUT! Processing product %d\n", target_idx);
        
        // Mark non-bidding players as bid = -1 (no bid)
        for (int i = 0; i < g_r2.player_count; i++) {
            if (!g_r2.players[i].has_bid) {
                g_r2.players[i].has_bid = true;
                g_r2.players[i].bid_value = -1;  // No bid
                printf("[Round2-Timer] Player %d did not bid in time\n", 
                       g_r2.players[i].account_id);
            }
        }
        
        // Process results and advance
        MessageHeader dummy = {0};
        process_turn_results(&dummy);
    }
    
    return NULL;
}

static void start_product_timer(int product_idx) {
    pthread_mutex_lock(&g_r2_mutex);
    
    g_r2.timer_running = false;
    g_r2.product_start_time = time(NULL);
    g_r2.current_timer_idx = product_idx;
    g_r2.timer_running = true;
    
    pthread_mutex_unlock(&g_r2_mutex);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    if (pthread_create(&g_r2.timer_thread, &attr, product_timer_thread, NULL) != 0) {
        printf("[Round2-Timer] Warning: Failed to create timer thread\n");
    }
    
    pthread_attr_destroy(&attr);
}

static void stop_product_timer(void) {
    pthread_mutex_lock(&g_r2_mutex);
    g_r2.timer_running = false;
    pthread_mutex_unlock(&g_r2_mutex);
}

//==============================================================================
// ELIMINATION LOGIC (same as Round 1)
//==============================================================================

typedef struct {
    int32_t account_id;
    int32_t score;
} ScoreEntry;

static void eliminate_player(MatchPlayerState *mp, const char *reason) {
    if (!mp) return;
    
    mp->eliminated = 1;
    mp->eliminated_at_round = g_r2.round_index + 1;
    
    printf("[Round2] Player %d ELIMINATED at round %d (reason: %s, score=%d)\n", 
           mp->account_id, g_r2.round_index + 1, reason, mp->score);
    
    // =========================================================================
    // SEND NTF_ELIMINATION to the eliminated player
    // =========================================================================
    UserSession *elim_session = session_get_by_account(mp->account_id);
    int elim_fd = elim_session ? elim_session->socket_fd : -1;
    
    if (elim_fd > 0) {
        cJSON *ntf = cJSON_CreateObject();
        cJSON_AddNumberToObject(ntf, "player_id", mp->account_id);
        cJSON_AddStringToObject(ntf, "reason", reason);
        cJSON_AddNumberToObject(ntf, "round", g_r2.round_index + 1);
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
            printf("[Round2] Sent NTF_ELIMINATION to player %d\n", mp->account_id);
            free(ntf_json);
        }
        
        // Change session state back to LOBBY so player can join new games
        session_mark_lobby(elim_session);
        printf("[Round2] Changed player %d session state to LOBBY\n", mp->account_id);
    }
    
    // =========================================================================
    // Save elimination event to database
    // =========================================================================
    MatchState *match = get_match();
    if (mp->account_id > 0 && match && match->db_match_id > 0) {
        // Use db_match_event_insert from match_repo
        // Note: player_id in match_events refers to accounts.id, not match_players.id
        db_error_t event_err = db_match_event_insert(
            match->db_match_id,
            mp->account_id,  // Use account_id, not match_player_id
            "ELIMINATED",
            g_r2.round_index + 1,
            0
        );
        if (event_err == DB_SUCCESS) {
            printf("[Round2] Saved elimination event to DB\n");
        } else {
            printf("[Round2] Failed to save elimination event: err=%d\n", event_err);
        }
        
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

// Returns true if bonus round was triggered
static bool perform_elimination(void) {
    MatchState *match = get_match();
    if (!match) return false;
    
    // MODE_SCORING: No elimination
    if (match->mode == MODE_SCORING) {
        printf("[Round2] Scoring mode - disconnected players get 0 points\n");
        for (int i = 0; i < g_r2.player_count; i++) {
            int32_t acc_id = g_r2.players[i].account_id;
            if (!is_connected(acc_id)) {
                printf("[Round2] Player %d disconnected - 0 points (can rejoin next round)\n", acc_id);
            }
        }
        printf("[Round2] Scoring mode - no elimination\n");
        return false;
    }
    
    // MODE_ELIMINATION: Eliminate disconnected + lowest scorer
    printf("[Round2] Elimination mode - processing...\n");
    
    // Step 1: Eliminate all disconnected players
    for (int i = 0; i < g_r2.player_count; i++) {
        int32_t acc_id = g_r2.players[i].account_id;
        MatchPlayerState *mp = get_match_player(acc_id);
        
        if (mp && !mp->eliminated && !is_connected(acc_id)) {
            eliminate_player(mp, "DISCONNECT");
        }
    }
    
    // Step 2: Find lowest scorer among connected players
    ScoreEntry scores[MAX_MATCH_PLAYERS];
    int count = 0;
    
    for (int i = 0; i < g_r2.player_count; i++) {
        MatchPlayerState *mp = get_match_player(g_r2.players[i].account_id);
        if (mp && !mp->eliminated && is_connected(mp->account_id)) {
            scores[count].account_id = mp->account_id;
            scores[count].score = mp->score;
            count++;
        }
    }
    
    if (count < 2) {
        printf("[Round2] Only %d connected players remaining, no further elimination\n", count);
        return false;
    }
    
    // Sort ascending
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
        printf("[Round2] %d players tied at lowest (%d), triggering bonus round\n", tie_count, lowest);
        
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
            check_and_trigger_bonus(match->runtime_match_id, 2);
            return true;  // Bonus triggered
        }
        return false;
    }
    
    // Eliminate lowest scorer
    MatchPlayerState *elim_player = get_match_player(scores[0].account_id);
    if (elim_player) {
        eliminate_player(elim_player, "LOWEST_SCORE");
    }
    
    return false;
}

//==============================================================================
// HELPER: Build round end result
//==============================================================================

static char* build_round_end_json(void) {
    MatchState *match = get_match();
    if (!match) return NULL;
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddTrueToObject(obj, "success");
    cJSON_AddNumberToObject(obj, "match_id", g_r2.match_id);
    cJSON_AddNumberToObject(obj, "round", 2);
    
    int connected = count_connected();
    int disconnected = count_disconnected();
    
    cJSON_AddNumberToObject(obj, "connected_count", connected);
    cJSON_AddNumberToObject(obj, "disconnected_count", disconnected);
    cJSON_AddNumberToObject(obj, "finished_count", g_r2.player_count);
    cJSON_AddNumberToObject(obj, "player_count", g_r2.player_count);
    
    bool can_continue = (disconnected < 2);
    cJSON_AddBoolToObject(obj, "can_continue", can_continue);
    
    if (disconnected >= 2) {
        cJSON_AddStringToObject(obj, "status_message", "Game ended - too many disconnections");
    } else if (disconnected == 1) {
        cJSON_AddStringToObject(obj, "status_message", "1 disconnected - no elimination");
    } else {
        cJSON_AddStringToObject(obj, "status_message", "Round completed");
    }
    
    // Build rankings (sorted by score descending)
    ScoreEntry scores[MAX_MATCH_PLAYERS];
    int count = 0;
    for (int i = 0; i < g_r2.player_count; i++) {
        MatchPlayerState *mp = get_match_player(g_r2.players[i].account_id);
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
        
        // Add player name
        cJSON_AddStringToObject(p, "name", mp->name);
        
        cJSON_AddItemToArray(players, p);
    }
    cJSON_AddItemToObject(obj, "players", players);
    cJSON_AddItemToObject(obj, "rankings", cJSON_Duplicate(players, 1));
    
    // Add next round info
    // current_round_idx was already advanced to the next round
    // For frontend, we need 1-based index
    int next_round = 0;
    if (match->current_round_idx < match->round_count) {
        next_round = match->current_round_idx + 1; // 1-based for frontend
    }
    cJSON_AddNumberToObject(obj, "next_round", next_round);
    cJSON_AddNumberToObject(obj, "current_round", 2);
    printf("[Round2] Build end JSON: current_round_idx=%d, round_count=%d, next_round=%d\n",
           match->current_round_idx, match->round_count, next_round);
    
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    return json;
}

//==============================================================================
// PROCESS: Turn results (after all bids received)
//==============================================================================

static void process_turn_results(MessageHeader *req) {
    stop_product_timer();
    
    RoundState *round = get_round();
    if (!round) return;
    
    int product_idx = round->current_question_idx;
    int64_t correct_price = get_correct_price(product_idx);
    
    printf("[Round2] Processing turn %d results (correct_price=%lld)\n", 
           product_idx, (long long)correct_price);
    
    // Build results for all players
    BidResult results[MAX_MATCH_PLAYERS];
    int result_count = 0;
    
    for (int i = 0; i < g_r2.player_count; i++) {
        MatchPlayerState *mp = get_match_player(g_r2.players[i].account_id);
        if (!mp || mp->eliminated) continue;
        
        results[result_count].account_id = g_r2.players[i].account_id;
        results[result_count].bid = g_r2.players[i].bid_value;
        result_count++;
    }
    
    // Calculate scores
    calculate_turn_scores(correct_price, results, result_count);
    
    // Apply scores to MatchPlayerState and save to DB
    cJSON *bids_array = cJSON_CreateArray();
    
    for (int i = 0; i < result_count; i++) {
        MatchPlayerState *mp = get_match_player(results[i].account_id);
        if (!mp) continue;
        
        mp->score += results[i].score;
        mp->score_delta = results[i].score;
        
        printf("[Round2] Player %d: bid=%lld score_delta=%d total=%d\n",
               results[i].account_id, (long long)results[i].bid, 
               results[i].score, mp->score);
        
        // Save to match_answer using db_match_answer_insert
        if (mp->match_player_id > 0 && round->questions[product_idx].question_id > 0) {
            // Build answer JSON string
            cJSON *ans_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(ans_obj, "bid", results[i].bid);
            cJSON_AddNumberToObject(ans_obj, "correct_price", correct_price);
            cJSON_AddBoolToObject(ans_obj, "overbid", results[i].over);
            char *ans_json = cJSON_PrintUnformatted(ans_obj);
            cJSON_Delete(ans_obj);
            
            db_error_t db_err = db_match_answer_insert(
                round->questions[product_idx].question_id,
                mp->match_player_id,
                ans_json,
                results[i].score,
                1  // action_idx = 1
            );
            if (db_err == DB_SUCCESS) {
                printf("[Round2] Saved bid to DB: p=%d bid=%lld\n", 
                       mp->match_player_id, (long long)results[i].bid);
            }
            if (ans_json) free(ans_json);
        }
        
        // Add to bids array for response
        cJSON *bid_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(bid_obj, "id", results[i].account_id);
        cJSON_AddNumberToObject(bid_obj, "bid", results[i].bid);
        cJSON_AddNumberToObject(bid_obj, "score_delta", results[i].score);
        cJSON_AddNumberToObject(bid_obj, "total_score", mp->score);
        cJSON_AddBoolToObject(bid_obj, "overbid", results[i].over);
        
        cJSON_AddStringToObject(bid_obj, "name", mp->name);
        
        cJSON_AddItemToArray(bids_array, bid_obj);
    }
    
    // Build turn result JSON
    cJSON *result = cJSON_CreateObject();
    cJSON_AddTrueToObject(result, "success");
    cJSON_AddNumberToObject(result, "product_idx", product_idx);
    cJSON_AddNumberToObject(result, "correct_price", correct_price);
    cJSON_AddItemToObject(result, "bids", bids_array);
    
    char *json = cJSON_PrintUnformatted(result);
    cJSON_Delete(result);
    
    printf("[Round2] Turn result JSON: %s\n", json);
    broadcast_json(req, OP_S2C_ROUND2_TURN_RESULT, json);
    free(json);
    
    // Mark question as ended
    if (product_idx < round->question_count) {
        round->questions[product_idx].status = QUESTION_ENDED;
    }
    
    // Wait a bit for clients to see result, then advance
    // Wait for clients to see result (match frontend's 3-second display)
    sleep(3);
    
    advance_to_next_product(req);
}

//==============================================================================
// ADVANCE: Move to next product or end round
//==============================================================================

static void advance_to_next_product(MessageHeader *req) {
    RoundState *round = get_round();
    if (!round) return;
    
    round->current_question_idx++;
    
    if (round->current_question_idx >= round->question_count) {
        // Round finished
        printf("[Round2] All products completed\n");
        round->status = ROUND_ENDED;
        round->ended_at = time(NULL);
        g_r2.is_active = false;
        
        // Perform elimination - returns true if bonus triggered
        bool bonus_triggered = perform_elimination();
        
        if (bonus_triggered) {
            printf("[Round2] Bonus round active - waiting for bonus to complete\n");
            return;
        }
        
        // Advance match to next round
        MatchState *match = get_match();
        if (match && match->current_round_idx < match->round_count - 1) {
            match->current_round_idx++;
            printf("[Round2] Advanced to round %d\n", match->current_round_idx + 1);
            
            // Initialize next round state
            RoundState *next_round = &match->rounds[match->current_round_idx];
            next_round->status = ROUND_PENDING;
            next_round->current_question_idx = 0;
        }
        
        // Broadcast final results
        char *end_json = build_round_end_json();
        if (end_json) {
            printf("[Round2] Round end JSON: %s\n", end_json);
            broadcast_json(req, OP_S2C_ROUND2_ALL_FINISHED, end_json);
            free(end_json);
        }
    } else {
        // Send next product
        reset_bids();
        broadcast_current_product(req);
    }
}

//==============================================================================
// BROADCAST: Send current product to all players
//==============================================================================

static void broadcast_current_product(MessageHeader *req) {
    RoundState *round = get_round();
    if (!round) return;
    
    int product_idx = round->current_question_idx;
    printf("[Round2] Broadcasting product %d/%d\n", product_idx + 1, round->question_count);
    
    reset_bids();
    
    if (product_idx < round->question_count) {
        round->questions[product_idx].status = QUESTION_ACTIVE;
    }
    
    char *json = build_product_json(product_idx);
    if (json) {
        printf("[Round2] Product data: %s\n", json);
        broadcast_json(req, OP_S2C_ROUND2_PRODUCT, json);
        free(json);
        
        start_product_timer(product_idx);
    } else {
        printf("[Round2] ERROR: Failed to build product JSON for idx=%d\n", product_idx);
    }
}

//==============================================================================
// HANDLER: Player Ready (OP_C2S_ROUND2_PLAYER_READY)
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
    
    printf("[Round2] Player ready: match=%u account=%d\n", match_id, account_id);
    
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
    if (match_id != g_r2.match_id) {
        reset_context();
        g_r2.match_id = match_id;
        g_r2.round_index = match->current_round_idx;
    }
    
    R2_PlayerBid *pb = add_player(account_id);
    if (!pb) {
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
        cJSON_AddNumberToObject(obj, "current_product", round->current_question_idx);
        
        // Add full player list for leaderboard
        cJSON *players = cJSON_CreateArray();
        for (int i = 0; i < g_r2.player_count; i++) {
            cJSON *p = cJSON_CreateObject();
            cJSON_AddNumberToObject(p, "account_id", g_r2.players[i].account_id);
            cJSON_AddBoolToObject(p, "ready", g_r2.players[i].ready);
            
            MatchPlayerState *mp_state = get_match_player(g_r2.players[i].account_id);
            if (mp_state) {
                cJSON_AddStringToObject(p, "name", mp_state->name);
                cJSON_AddNumberToObject(p, "score", mp_state->score);
            }
            cJSON_AddItemToArray(players, p);
        }
        cJSON_AddItemToObject(obj, "players", players);
        
        char *json = cJSON_PrintUnformatted(obj);
        cJSON_Delete(obj);
        send_json(fd, req, OP_S2C_ROUND2_READY_STATUS, json);
        free(json);
        
        char *p_json = build_product_json(round->current_question_idx);
        if (p_json) {
            send_json(fd, req, OP_S2C_ROUND2_PRODUCT, p_json);
            free(p_json);
        }
        return;
    }
    
    // Mark ready
    if (!pb->ready) {
        pb->ready = true;
        g_r2.ready_count++;
    }
    
    // Broadcast ready status
    cJSON *status = cJSON_CreateObject();
    cJSON_AddTrueToObject(status, "success");
    cJSON_AddNumberToObject(status, "ready_count", g_r2.ready_count);
    cJSON_AddNumberToObject(status, "player_count", g_r2.player_count);
    cJSON_AddNumberToObject(status, "required_players", match->player_count);
    
    cJSON *players = cJSON_CreateArray();
    for (int i = 0; i < g_r2.player_count; i++) {
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "account_id", g_r2.players[i].account_id);
        cJSON_AddBoolToObject(p, "ready", g_r2.players[i].ready);
        
        MatchPlayerState *mp_state = get_match_player(g_r2.players[i].account_id);
        if (mp_state) {
            cJSON_AddStringToObject(p, "name", mp_state->name);
        }
        
        cJSON_AddItemToArray(players, p);
    }
    cJSON_AddItemToObject(status, "players", players);
    
    char *json = cJSON_PrintUnformatted(status);
    cJSON_Delete(status);
    broadcast_json(req, OP_S2C_ROUND2_READY_STATUS, json);
    free(json);
    
    // Check if all ready
    // Calculate expected players (non-eliminated)
    int expected = 0;
    for (int i = 0; i < match->player_count; i++) {
        if (!match->players[i].eliminated) expected++;
    }
    
    printf("[Round2] Check start: ready=%d, expected=%d, round_status=%d\n",
           g_r2.ready_count, expected, round ? round->status : -1);
           
    // Start when all non-eliminated players are ready
    if (g_r2.ready_count >= expected && expected > 0 && round && round->status == ROUND_PENDING) {
        printf("[Round2] All ready, starting round\n");
        
        round->status = ROUND_PLAYING;
        round->started_at = time(NULL);
        round->current_question_idx = 0;
        g_r2.is_active = true;
        
        cJSON *start = cJSON_CreateObject();
        cJSON_AddTrueToObject(start, "success");
        cJSON_AddNumberToObject(start, "match_id", match_id);
        cJSON_AddNumberToObject(start, "round", 2);
        cJSON_AddNumberToObject(start, "total_products", round->question_count);
        cJSON_AddNumberToObject(start, "time_per_product_ms", TIME_PER_PRODUCT);
        cJSON_AddNumberToObject(start, "player_count", g_r2.player_count);
        
        char *start_json = cJSON_PrintUnformatted(start);
        cJSON_Delete(start);
        broadcast_json(req, OP_S2C_ROUND2_ALL_READY, start_json);
        free(start_json);
        
        // Send first product
        broadcast_current_product(req);
    }
}

//==============================================================================
// HANDLER: Get Product (OP_C2S_ROUND2_GET_PRODUCT)
//==============================================================================

static void handle_get_product(int fd, MessageHeader *req, const char *payload) {
    if (req->length < 8) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid payload\"}");
        return;
    }
    
    uint32_t match_id, product_idx;
    memcpy(&match_id, payload, 4);
    memcpy(&product_idx, payload + 4, 4);
    match_id = ntohl(match_id);
    product_idx = ntohl(product_idx);
    
    if (match_id != g_r2.match_id) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid match\"}");
        return;
    }
    
    RoundState *round = get_round();
    if (!round || round->status != ROUND_PLAYING) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Round not active\"}");
        return;
    }
    
    UserSession *session = session_get_by_socket(fd);
    if (session) {
        MatchPlayerState *mp = get_match_player(session->account_id);
        if (mp && mp->eliminated) {
            send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Player eliminated\"}");
            return;
        }
    }
    
    char *json = build_product_json(round->current_question_idx);
    if (!json) {
        send_json(fd, req, ERR_SERVER_ERROR, "{\"success\":false,\"error\":\"Product not found\"}");
        return;
    }
    
    send_json(fd, req, OP_S2C_ROUND2_PRODUCT, json);
    free(json);
}

//==============================================================================
// HANDLER: Submit Bid (OP_C2S_ROUND2_BID)
// Payload: match_id (4) + product_idx (4) + bid_value (8) = 16 bytes
//==============================================================================

static void handle_submit_bid(int fd, MessageHeader *req, const char *payload) {
    if (req->length < 16) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid payload\"}");
        return;
    }
    
    uint32_t match_id, product_idx;
    int64_t bid_value;
    
    memcpy(&match_id, payload, 4);
    memcpy(&product_idx, payload + 4, 4);
    memcpy(&bid_value, payload + 8, 8);
    
    match_id = ntohl(match_id);
    product_idx = ntohl(product_idx);
    bid_value = (int64_t)be64toh((uint64_t)bid_value);
    
    printf("[Round2] Bid: product=%u bid=%lld\n", product_idx, (long long)bid_value);
    
    if (match_id != g_r2.match_id) {
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
    
    R2_PlayerBid *pb = find_player(session->account_id);
    if (!pb) {
        send_json(fd, req, ERR_SERVER_ERROR, "{\"success\":false,\"error\":\"Player not in round\"}");
        return;
    }
    
    if ((int)product_idx != round->current_question_idx) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Wrong product\"}");
        return;
    }
    
    if (pb->has_bid) {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Already bid\"}");
        return;
    }
    
    // Store bid
    pb->bid_value = bid_value;
    pb->has_bid = true;
    
    printf("[Round2] Player %d bid %lld for product %d\n", 
           session->account_id, (long long)bid_value, product_idx);
    
    // Send acknowledgment
    cJSON *ack = cJSON_CreateObject();
    cJSON_AddTrueToObject(ack, "success");
    cJSON_AddNumberToObject(ack, "product_idx", product_idx);
    cJSON_AddNumberToObject(ack, "bid", bid_value);
    
    int bid_count = count_bid();
    int connected = count_connected();
    cJSON_AddNumberToObject(ack, "bid_count", bid_count);
    cJSON_AddNumberToObject(ack, "player_count", connected);
    
    bool all_bid = (bid_count >= connected);
    cJSON_AddBoolToObject(ack, "all_bid", all_bid);
    
    char *json = cJSON_PrintUnformatted(ack);
    cJSON_Delete(ack);
    send_json(fd, req, OP_S2C_ROUND2_BID_ACK, json);
    free(json);
    
    // If all bid, process results
    if (all_bid) {
        printf("[Round2] All %d players bid for product %d\n", connected, product_idx);
        process_turn_results(req);
    }
}

//==============================================================================
// DISCONNECT HANDLER
//==============================================================================

void handle_round2_disconnect(int client_fd) {
    UserSession *session = session_get_by_socket(client_fd);
    if (!session) return;
    
    int32_t account_id = session->account_id;
    R2_PlayerBid *pb = find_player(account_id);
    if (!pb) return;
    
    printf("[Round2] Player %d disconnected\n", account_id);
    
    MatchPlayerState *mp = get_match_player(account_id);
    if (mp) {
        mp->connected = 0;
    }
    
    int connected = count_connected();
    int disconnected = count_disconnected();
    
    // Notify others
    cJSON *ntf = cJSON_CreateObject();
    cJSON_AddNumberToObject(ntf, "player_id", account_id);
    cJSON_AddStringToObject(ntf, "event", "disconnect");
    cJSON_AddNumberToObject(ntf, "connected_count", connected);
    cJSON_AddNumberToObject(ntf, "disconnected_count", disconnected);
    
    if (disconnected >= 2) {
        cJSON_AddTrueToObject(ntf, "game_ended");
        cJSON_AddStringToObject(ntf, "reason", "Too many disconnections");
    }
    
    char *json = cJSON_PrintUnformatted(ntf);
    cJSON_Delete(ntf);
    
    MessageHeader hdr = {0};
    hdr.magic = htons(MAGIC_NUMBER);
    hdr.version = PROTOCOL_VERSION;
    hdr.command = htons(NTF_PLAYER_LEFT);
    hdr.length = htonl((uint32_t)strlen(json));
    
    for (int i = 0; i < g_r2.player_count; i++) {
        if (g_r2.players[i].account_id != account_id) {
            int fd = get_socket(g_r2.players[i].account_id);
            if (fd > 0) {
                send(fd, &hdr, sizeof(hdr), 0);
                send(fd, json, strlen(json), 0);
            }
        }
    }
    free(json);
    
    // Check if should end game
    RoundState *round = get_round();
    if (round && round->status == ROUND_PLAYING && disconnected >= 2) {
        printf("[Round2] Too many disconnections, ending game\n");
        
        round->status = ROUND_ENDED;
        round->ended_at = time(NULL);
        g_r2.is_active = false;
        
        MatchState *match = get_match();
        if (match) {
            match->status = MATCH_ENDED;
        }
        
        cJSON *end = cJSON_CreateObject();
        cJSON_AddFalseToObject(end, "success");
        cJSON_AddStringToObject(end, "error", "Too many disconnections");
        
        char *ejson = cJSON_PrintUnformatted(end);
        cJSON_Delete(end);
        
        hdr.command = htons(OP_S2C_ROUND2_ALL_FINISHED);
        hdr.length = htonl((uint32_t)strlen(ejson));
        
        for (int i = 0; i < g_r2.player_count; i++) {
            int fd = get_socket(g_r2.players[i].account_id);
            if (fd > 0) {
                send(fd, &hdr, sizeof(hdr), 0);
                send(fd, ejson, strlen(ejson), 0);
            }
        }
        free(ejson);
    }
    
    // Check if all remaining players have bid → process
    if (round && round->status == ROUND_PLAYING) {
        int bid_count = count_bid();
        if (bid_count >= connected && connected > 0) {
            printf("[Round2] All remaining players bid, processing\n");
            MessageHeader dummy = {0};
            process_turn_results(&dummy);
        }
    }
}

//==============================================================================
// MAIN DISPATCHER
//==============================================================================

void handle_round2(int client_fd, MessageHeader *req_header, const char *payload) {
    printf("[Round2] cmd=0x%04X len=%u\n", req_header->command, req_header->length);
    
    switch (req_header->command) {
        case OP_C2S_ROUND2_READY:
        case OP_C2S_ROUND2_PLAYER_READY:
            handle_player_ready(client_fd, req_header, payload);
            break;
        case OP_C2S_ROUND2_GET_PRODUCT:
            handle_get_product(client_fd, req_header, payload);
            break;
        case OP_C2S_ROUND2_BID:
            handle_submit_bid(client_fd, req_header, payload);
            break;
        default:
            printf("[Round2] Unknown cmd: 0x%04X\n", req_header->command);
            break;
    }
}
