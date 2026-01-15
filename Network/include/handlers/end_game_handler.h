/**
 * end_game_handler.h - End Game Handler
 * 
 * PURPOSE:
 * Handles the end game state for both Scoring and Elimination modes.
 * Calculates final rankings, determines winner(s), and broadcasts results.
 * 
 * SCORING MODE:
 * - Case 1: No bonus round after Round 3
 *   - Game ends immediately, rank by score
 *   - If tie at top, display "Multiple players tied"
 * - Case 2: Has bonus round after Round 3
 *   - Bonus round winner = TOP 1
 *   - Others ranked by score (no bonus points added)
 * 
 * ELIMINATION MODE:
 * - After Round 3, only 1 player remains
 * - Winner = last player standing
 * - Rankings: TOP 1 = winner, TOP 2-4 = eliminated players by round
 */

#ifndef END_GAME_HANDLER_H
#define END_GAME_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol/protocol.h"
#include "handlers/match_manager.h"
#include "transport/room_manager.h"  // For GameMode, MODE_SCORING, MODE_ELIMINATION

//==============================================================================
// ENUMS
//==============================================================================

typedef enum {
    END_GAME_REASON_ROUND3_COMPLETE = 0,  // Normal completion after Round 3
    END_GAME_REASON_BONUS_WINNER,          // Winner decided by bonus round
    END_GAME_REASON_ELIMINATION_WINNER,    // Last player standing in elimination
    END_GAME_REASON_FORFEIT,               // All other players forfeited
    END_GAME_REASON_DISCONNECT             // Too many disconnections
} EndGameReason;

typedef enum {
    RANKING_BY_SCORE = 0,         // Ranked by final score (Scoring mode)
    RANKING_BY_ELIMINATION,       // Ranked by elimination round (Elimination mode)
    RANKING_BY_BONUS              // Winner decided by bonus round
} RankingType;

//==============================================================================
// STRUCTURES
//==============================================================================

typedef struct {
    int32_t account_id;
    char name[64];
    int32_t score;
    int rank;                     // 1 = TOP 1, 2 = TOP 2, etc.
    bool is_winner;
    bool eliminated;
    int eliminated_at_round;      // 0 if not eliminated, 1-3 for round eliminated
    bool bonus_winner;            // True if won via bonus round
} EndGamePlayerRanking;

typedef struct {
    uint32_t match_id;
    GameMode mode;                // MODE_SCORING or MODE_ELIMINATION
    EndGameReason reason;
    RankingType ranking_type;
    
    EndGamePlayerRanking rankings[MAX_MATCH_PLAYERS];
    int player_count;
    
    bool has_tie_at_top;          // True if multiple players tied at highest score
    int32_t winner_id;            // -1 if tie, otherwise winner's account_id
    char winner_name[64];
    int32_t winner_score;
    
    bool bonus_round_played;      // True if bonus round was played
} EndGameResult;

//==============================================================================
// API FUNCTIONS
//==============================================================================

/**
 * Trigger end game after Round 3 completes
 * @param match_id Runtime match ID
 * @param bonus_winner_id If bonus round was played, the winner's account_id (-1 if no bonus)
 * @return true if end game was triggered successfully
 */
bool trigger_end_game(uint32_t match_id, int32_t bonus_winner_id);

/**
 * Build end game result for a match
 * @param match_id Runtime match ID
 * @param bonus_winner_id Winner from bonus round (-1 if no bonus)
 * @param result Output structure to fill
 * @return true if result was built successfully
 */
bool build_end_game_result(uint32_t match_id, int32_t bonus_winner_id, EndGameResult *result);

/**
 * Broadcast end game result to all players in match
 * @param result The end game result to broadcast
 * @param req Message header for response
 */
void broadcast_end_game_result(EndGameResult *result, MessageHeader *req);

/**
 * Handle player requesting to go back to lobby
 * @param match_id Runtime match ID
 * @param account_id Player's account ID
 */
void handle_end_game_back_to_lobby(uint32_t match_id, int32_t account_id);

/**
 * Main dispatcher for end game commands
 */
void handle_end_game(int client_fd, MessageHeader *req_header, const char *payload);

/**
 * Cleanup end game state for a match
 */
void cleanup_end_game(uint32_t match_id);

#endif // END_GAME_HANDLER_H
