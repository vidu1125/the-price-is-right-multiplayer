/**
 * end_game_handler.c - End Game Handler Implementation
 * 
 * PURPOSE:
 * Handles the end game state for both Scoring and Elimination modes.
 * Calculates final rankings, determines winner(s), and broadcasts results.
 * 
 * SCORING MODE:
 * - Case 1: No bonus round after Round 3
 *   - Game ends immediately, rank by score (high to low)
 *   - If tie at top, display "Multiple players tied"
 * - Case 2: Has bonus round after Round 3
 *   - Bonus round winner = TOP 1 (unique)
 *   - Others ranked by score (bonus round does NOT add points)
 * 
 * ELIMINATION MODE:
 * - 4 players, 3 rounds, 1 elimination per round
 * - After Round 3: only 1 player remains = WINNER
 * - Rankings:
 *   - TOP 1: Winner (survivor)
 *   - TOP 2: Eliminated in Round 3
 *   - TOP 3: Eliminated in Round 2
 *   - TOP 4: Eliminated in Round 1
 * - NO bonus round in Elimination mode (always exactly 1 survivor)
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <time.h>

#include "handlers/end_game_handler.h"
#include "handlers/session_manager.h"
#include "handlers/match_manager.h"
#include "transport/room_manager.h"  // For GameMode, MODE_SCORING, MODE_ELIMINATION
#include "db/core/db_client.h"
#include "db/repo/match_repo.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include <cjson/cJSON.h>

//==============================================================================
// HELPER: Get socket for account
//==============================================================================
static int get_socket(int32_t account_id) {
    UserSession *session = session_get_by_account(account_id);
    if (session && session->state != SESSION_PLAYING_DISCONNECTED) {
        return session->socket_fd;
    }
    return -1;
}

//==============================================================================
// HELPER: Send JSON response
//==============================================================================
static void send_json(int fd, MessageHeader *req, uint16_t cmd, const char *json) {
    if (fd <= 0 || !json) return;
    forward_response(fd, req, cmd, json, (uint32_t)strlen(json));
}

//==============================================================================
// HELPER: Compare function for sorting by score (descending)
//==============================================================================
static int compare_by_score_desc(const void *a, const void *b) {
    const EndGamePlayerRanking *pa = (const EndGamePlayerRanking *)a;
    const EndGamePlayerRanking *pb = (const EndGamePlayerRanking *)b;
    return pb->score - pa->score;  // Descending
}

//==============================================================================
// HELPER: Compare function for sorting by elimination round (ascending = later elimination = higher rank)
//==============================================================================
static int compare_by_elimination(const void *a, const void *b) {
    const EndGamePlayerRanking *pa = (const EndGamePlayerRanking *)a;
    const EndGamePlayerRanking *pb = (const EndGamePlayerRanking *)b;
    
    // Non-eliminated (winner) comes first
    if (!pa->eliminated && pb->eliminated) return -1;
    if (pa->eliminated && !pb->eliminated) return 1;
    
    // Both eliminated: later round = higher rank (lower rank number)
    // Round 3 > Round 2 > Round 1
    return pb->eliminated_at_round - pa->eliminated_at_round;
}

//==============================================================================
// BUILD END GAME RESULT - SCORING MODE
//==============================================================================
static bool build_scoring_mode_result(MatchState *match, int32_t bonus_winner_id, EndGameResult *result) {
    result->mode = MODE_SCORING;
    result->player_count = match->player_count;
    
    // Collect all players
    for (int i = 0; i < match->player_count; i++) {
        MatchPlayerState *mp = &match->players[i];
        EndGamePlayerRanking *r = &result->rankings[i];
        
        r->account_id = mp->account_id;
        snprintf(r->name, sizeof(r->name), "Player%d", mp->account_id);
        r->score = mp->score;
        r->eliminated = mp->eliminated != 0;
        r->eliminated_at_round = mp->eliminated_at_round;
        r->is_winner = false;
        r->bonus_winner = false;
        r->rank = 0;  // Will be set after sorting
    }
    
    // Check if bonus round was played
    if (bonus_winner_id > 0) {
        // CASE 2: Bonus round was played
        result->bonus_round_played = true;
        result->ranking_type = RANKING_BY_BONUS;
        result->reason = END_GAME_REASON_BONUS_WINNER;
        result->has_tie_at_top = false;
        
        // Find bonus winner and mark as TOP 1
        for (int i = 0; i < result->player_count; i++) {
            if (result->rankings[i].account_id == bonus_winner_id) {
                result->rankings[i].is_winner = true;
                result->rankings[i].bonus_winner = true;
                result->rankings[i].rank = 1;
                
                result->winner_id = bonus_winner_id;
                strncpy(result->winner_name, result->rankings[i].name, sizeof(result->winner_name));
                result->winner_score = result->rankings[i].score;
                break;
            }
        }
        
        // Sort remaining players by score for ranks 2+
        // First, separate winner from others
        EndGamePlayerRanking others[MAX_MATCH_PLAYERS];
        int others_count = 0;
        
        for (int i = 0; i < result->player_count; i++) {
            if (result->rankings[i].account_id != bonus_winner_id) {
                others[others_count++] = result->rankings[i];
            }
        }
        
        // Sort others by score descending
        qsort(others, others_count, sizeof(EndGamePlayerRanking), compare_by_score_desc);
        
        // Assign ranks 2, 3, 4...
        for (int i = 0; i < others_count; i++) {
            others[i].rank = i + 2;  // Start from rank 2
        }
        
        // Rebuild rankings array: winner first, then others
        int idx = 0;
        for (int i = 0; i < result->player_count; i++) {
            if (result->rankings[i].account_id == bonus_winner_id) {
                // Keep winner at position 0
                EndGamePlayerRanking winner = result->rankings[i];
                result->rankings[0] = winner;
                idx = 1;
                break;
            }
        }
        for (int i = 0; i < others_count; i++) {
            result->rankings[idx++] = others[i];
        }
        
    } else {
        // CASE 1: No bonus round - rank by score
        result->bonus_round_played = false;
        result->ranking_type = RANKING_BY_SCORE;
        result->reason = END_GAME_REASON_ROUND3_COMPLETE;
        
        // Sort all players by score descending
        qsort(result->rankings, result->player_count, sizeof(EndGamePlayerRanking), compare_by_score_desc);
        
        // Assign ranks
        for (int i = 0; i < result->player_count; i++) {
            result->rankings[i].rank = i + 1;
        }
        
        // Check for tie at top
        if (result->player_count >= 2 && 
            result->rankings[0].score == result->rankings[1].score) {
            // Multiple players tied at highest score
            result->has_tie_at_top = true;
            result->winner_id = -1;  // No single winner
            strcpy(result->winner_name, "TIE");
            result->winner_score = result->rankings[0].score;
            
            // Mark all tied players
            int top_score = result->rankings[0].score;
            for (int i = 0; i < result->player_count; i++) {
                if (result->rankings[i].score == top_score) {
                    result->rankings[i].is_winner = true;  // Co-winners
                    result->rankings[i].rank = 1;  // All tied at rank 1
                }
            }
        } else {
            // Single winner
            result->has_tie_at_top = false;
            result->rankings[0].is_winner = true;
            result->winner_id = result->rankings[0].account_id;
            strncpy(result->winner_name, result->rankings[0].name, sizeof(result->winner_name));
            result->winner_score = result->rankings[0].score;
        }
    }
    
    return true;
}

//==============================================================================
// BUILD END GAME RESULT - ELIMINATION MODE
//==============================================================================
static bool build_elimination_mode_result(MatchState *match, EndGameResult *result) {
    result->mode = MODE_ELIMINATION;
    result->player_count = match->player_count;
    result->bonus_round_played = false;  // Never bonus in elimination
    result->ranking_type = RANKING_BY_ELIMINATION;
    result->reason = END_GAME_REASON_ELIMINATION_WINNER;
    result->has_tie_at_top = false;
    
    // Collect all players
    for (int i = 0; i < match->player_count; i++) {
        MatchPlayerState *mp = &match->players[i];
        EndGamePlayerRanking *r = &result->rankings[i];
        
        r->account_id = mp->account_id;
        snprintf(r->name, sizeof(r->name), "Player%d", mp->account_id);
        r->score = mp->score;
        r->eliminated = mp->eliminated != 0;
        r->eliminated_at_round = mp->eliminated_at_round;
        r->is_winner = !mp->eliminated;  // Survivor is winner
        r->bonus_winner = false;
        r->rank = 0;
    }
    
    // Sort by elimination order:
    // - Winner (not eliminated) = TOP 1
    // - Eliminated Round 3 = TOP 2
    // - Eliminated Round 2 = TOP 3
    // - Eliminated Round 1 = TOP 4
    qsort(result->rankings, result->player_count, sizeof(EndGamePlayerRanking), compare_by_elimination);
    
    // Assign ranks
    for (int i = 0; i < result->player_count; i++) {
        result->rankings[i].rank = i + 1;
    }
    
    // Set winner info (should be rank 1)
    if (result->player_count > 0 && result->rankings[0].is_winner) {
        result->winner_id = result->rankings[0].account_id;
        strncpy(result->winner_name, result->rankings[0].name, sizeof(result->winner_name));
        result->winner_score = result->rankings[0].score;
    }
    
    return true;
}

//==============================================================================
// BUILD END GAME RESULT (Main function)
//==============================================================================
bool build_end_game_result(uint32_t match_id, int32_t bonus_winner_id, EndGameResult *result) {
    MatchState *match = match_get_by_id(match_id);
    if (!match) {
        printf("[EndGame] Match %u not found\n", match_id);
        return false;
    }
    
    memset(result, 0, sizeof(EndGameResult));
    result->match_id = match_id;
    
    printf("[EndGame] Building result for match %u, mode=%d, bonus_winner=%d\n",
           match_id, match->mode, bonus_winner_id);
    
    if (match->mode == MODE_ELIMINATION) {
        return build_elimination_mode_result(match, result);
    } else {
        return build_scoring_mode_result(match, bonus_winner_id, result);
    }
}

//==============================================================================
// BUILD JSON FOR END GAME RESULT
//==============================================================================
static char* build_end_game_json(EndGameResult *result) {
    cJSON *obj = cJSON_CreateObject();
    
    // Basic info
    cJSON_AddTrueToObject(obj, "success");
    cJSON_AddNumberToObject(obj, "match_id", result->match_id);
    cJSON_AddStringToObject(obj, "mode", result->mode == MODE_ELIMINATION ? "elimination" : "scoring");
    cJSON_AddStringToObject(obj, "reason", 
        result->reason == END_GAME_REASON_BONUS_WINNER ? "bonus_winner" :
        result->reason == END_GAME_REASON_ELIMINATION_WINNER ? "elimination_winner" :
        result->reason == END_GAME_REASON_FORFEIT ? "forfeit" :
        result->reason == END_GAME_REASON_DISCONNECT ? "disconnect" : "round3_complete");
    
    // Tie info (Scoring mode only)
    cJSON_AddBoolToObject(obj, "has_tie", result->has_tie_at_top);
    cJSON_AddBoolToObject(obj, "bonus_round_played", result->bonus_round_played);
    
    // Winner info
    cJSON *winner_obj = cJSON_CreateObject();
    if (result->has_tie_at_top) {
        cJSON_AddNumberToObject(winner_obj, "id", -1);
        cJSON_AddStringToObject(winner_obj, "name", "TIED");
        cJSON_AddStringToObject(winner_obj, "message", "Multiple players tied at highest score!");
    } else {
        cJSON_AddNumberToObject(winner_obj, "id", result->winner_id);
        cJSON_AddStringToObject(winner_obj, "name", result->winner_name);
        cJSON_AddNumberToObject(winner_obj, "score", result->winner_score);
        cJSON_AddBoolToObject(winner_obj, "bonus_winner", result->bonus_round_played);
    }
    cJSON_AddItemToObject(obj, "winner", winner_obj);
    
    // Rankings array
    cJSON *rankings = cJSON_CreateArray();
    for (int i = 0; i < result->player_count; i++) {
        EndGamePlayerRanking *r = &result->rankings[i];
        
        cJSON *player = cJSON_CreateObject();
        cJSON_AddNumberToObject(player, "account_id", r->account_id);
        cJSON_AddStringToObject(player, "name", r->name);
        cJSON_AddNumberToObject(player, "score", r->score);
        cJSON_AddNumberToObject(player, "rank", r->rank);
        cJSON_AddBoolToObject(player, "is_winner", r->is_winner);
        cJSON_AddBoolToObject(player, "eliminated", r->eliminated);
        cJSON_AddNumberToObject(player, "eliminated_at_round", r->eliminated_at_round);
        cJSON_AddBoolToObject(player, "bonus_winner", r->bonus_winner);
        
        // Status text for UI
        if (r->is_winner && !result->has_tie_at_top) {
            if (r->bonus_winner) {
                cJSON_AddStringToObject(player, "status", "WINNER (Bonus Round)");
            } else if (result->mode == MODE_ELIMINATION) {
                cJSON_AddStringToObject(player, "status", "WINNER (Last Standing)");
            } else {
                cJSON_AddStringToObject(player, "status", "WINNER");
            }
        } else if (r->eliminated && result->mode == MODE_ELIMINATION) {
            char status[64];
            snprintf(status, sizeof(status), "Eliminated in Round %d", r->eliminated_at_round);
            cJSON_AddStringToObject(player, "status", status);
        } else if (result->has_tie_at_top && r->rank == 1) {
            cJSON_AddStringToObject(player, "status", "TIED FOR 1ST");
        } else {
            cJSON_AddStringToObject(player, "status", "");
        }
        
        cJSON_AddItemToArray(rankings, player);
    }
    cJSON_AddItemToObject(obj, "rankings", rankings);
    
    // Message for UI
    if (result->has_tie_at_top) {
        cJSON_AddStringToObject(obj, "message", "Multiple players tied at highest score!");
    } else if (result->bonus_round_played) {
        char msg[128];
        snprintf(msg, sizeof(msg), "%s wins via Bonus Round!", result->winner_name);
        cJSON_AddStringToObject(obj, "message", msg);
    } else if (result->mode == MODE_ELIMINATION) {
        char msg[128];
        snprintf(msg, sizeof(msg), "%s is the last player standing!", result->winner_name);
        cJSON_AddStringToObject(obj, "message", msg);
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "%s wins with %d points!", result->winner_name, result->winner_score);
        cJSON_AddStringToObject(obj, "message", msg);
    }
    
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    return json;
}

//==============================================================================
// BROADCAST END GAME RESULT
//==============================================================================
void broadcast_end_game_result(EndGameResult *result, MessageHeader *req) {
    char *json = build_end_game_json(result);
    if (!json) {
        printf("[EndGame] Failed to build JSON\n");
        return;
    }
    
    printf("[EndGame] Broadcasting result: %s\n", json);
    
    // Send to all players in the match
    MatchState *match = match_get_by_id(result->match_id);
    if (match) {
        for (int i = 0; i < match->player_count; i++) {
            int fd = get_socket(match->players[i].account_id);
            if (fd > 0) {
                forward_response(fd, req, OP_S2C_END_GAME_RESULT, json, (uint32_t)strlen(json));
            }
        }
    }
    
    free(json);
}

//==============================================================================
// TRIGGER END GAME
//==============================================================================
bool trigger_end_game(uint32_t match_id, int32_t bonus_winner_id) {
    printf("[EndGame] Triggering end game for match %u (bonus_winner=%d)\n", 
           match_id, bonus_winner_id);
    
    EndGameResult result;
    if (!build_end_game_result(match_id, bonus_winner_id, &result)) {
        printf("[EndGame] Failed to build end game result\n");
        return false;
    }
    
    // Create dummy header for broadcast
    MessageHeader dummy_hdr = {0};
    dummy_hdr.magic = htons(MAGIC_NUMBER);
    dummy_hdr.version = PROTOCOL_VERSION;
    
    // Broadcast result
    broadcast_end_game_result(&result, &dummy_hdr);
    
    // Update match status
    MatchState *match = match_get_by_id(match_id);
    if (match) {
        match->status = MATCH_ENDED;
        match->ended_at = time(NULL);
        
        // Update winner in database
        if (match->db_match_id > 0 && result.winner_id > 0) {
            // Find winner's match_player_id
            for (int i = 0; i < match->player_count; i++) {
                if (match->players[i].account_id == result.winner_id) {
                    cJSON *update = cJSON_CreateObject();
                    cJSON_AddBoolToObject(update, "winner", true);
                    
                    char filter[64];
                    snprintf(filter, sizeof(filter), "id=eq.%d", match->players[i].match_player_id);
                    
                    cJSON *db_result = NULL;
                    db_patch("match_players", filter, update, &db_result);
                    cJSON_Delete(update);
                    if (db_result) cJSON_Delete(db_result);
                    break;
                }
            }
            
            // Update match end time
            cJSON *match_update = cJSON_CreateObject();
            char end_time[32];
            time_t now = time(NULL);
            strftime(end_time, sizeof(end_time), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
            cJSON_AddStringToObject(match_update, "ended_at", end_time);
            
            char match_filter[64];
            snprintf(match_filter, sizeof(match_filter), "id=eq.%lld", (long long)match->db_match_id);
            
            cJSON *match_result = NULL;
            db_patch("matches", match_filter, match_update, &match_result);
            cJSON_Delete(match_update);
            if (match_result) cJSON_Delete(match_result);
        }
    }
    
    printf("[EndGame] End game triggered successfully\n");
    return true;
}

//==============================================================================
// HANDLE BACK TO LOBBY
//==============================================================================
void handle_end_game_back_to_lobby(uint32_t match_id, int32_t account_id) {
    printf("[EndGame] Player %d requesting back to lobby from match %u\n", 
           account_id, match_id);
    
    UserSession *session = session_get_by_account(account_id);
    if (!session) {
        printf("[EndGame] Session not found for account %d\n", account_id);
        return;
    }
    
    // Mark session as LOBBY
    session_mark_lobby(session);
    
    // Send acknowledgment
    int fd = session->socket_fd;
    if (fd > 0) {
        cJSON *ack = cJSON_CreateObject();
        cJSON_AddTrueToObject(ack, "success");
        cJSON_AddStringToObject(ack, "message", "Returning to lobby");
        
        char *json = cJSON_PrintUnformatted(ack);
        cJSON_Delete(ack);
        
        if (json) {
            MessageHeader dummy_hdr = {0};
            dummy_hdr.magic = htons(MAGIC_NUMBER);
            dummy_hdr.version = PROTOCOL_VERSION;
            
            forward_response(fd, &dummy_hdr, OP_S2C_END_GAME_BACK_ACK, json, (uint32_t)strlen(json));
            free(json);
        }
    }
    
    printf("[EndGame] Player %d session marked as LOBBY\n", account_id);
}

//==============================================================================
// HANDLE END GAME READY
//==============================================================================
static void handle_end_game_ready(int fd, MessageHeader *req, const char *payload) {
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
    
    printf("[EndGame] Player %d ready for end game, match %u\n", session->account_id, match_id);
    
    // Build and send current end game result
    EndGameResult result;
    if (build_end_game_result(match_id, -1, &result)) {
        char *json = build_end_game_json(&result);
        if (json) {
            send_json(fd, req, OP_S2C_END_GAME_RESULT, json);
            free(json);
        }
    } else {
        send_json(fd, req, ERR_BAD_REQUEST, "{\"success\":false,\"error\":\"Match not found\"}");
    }
}

//==============================================================================
// HANDLE BACK TO LOBBY REQUEST
//==============================================================================
static void handle_back_to_lobby(int fd, MessageHeader *req, const char *payload) {
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
    
    handle_end_game_back_to_lobby(match_id, session->account_id);
}

//==============================================================================
// MAIN DISPATCHER
//==============================================================================
void handle_end_game(int client_fd, MessageHeader *req_header, const char *payload) {
    printf("[EndGame] cmd=0x%04X len=%u\n", req_header->command, req_header->length);
    
    switch (req_header->command) {
        case OP_C2S_END_GAME_READY:
            handle_end_game_ready(client_fd, req_header, payload);
            break;
        case OP_C2S_END_GAME_BACK_LOBBY:
            handle_back_to_lobby(client_fd, req_header, payload);
            break;
        default:
            printf("[EndGame] Unknown cmd: 0x%04X\n", req_header->command);
            break;
    }
}

//==============================================================================
// CLEANUP
//==============================================================================
void cleanup_end_game(uint32_t match_id) {
    printf("[EndGame] Cleanup for match %u\n", match_id);
    // Nothing to clean up currently - state is in MatchState
}
