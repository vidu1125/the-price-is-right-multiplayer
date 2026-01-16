#ifndef BONUS_HANDLER_H
#define BONUS_HANDLER_H

#include "protocol/protocol.h"
#include "handlers/start_game_handler.h"

/**
 * bonus_handler.h - Bonus Round (Tiebreaker)
 * 
 * PURPOSE:
 * Bonus Round is triggered when there's a tie at critical positions:
 * - After Round 1/2: Multiple players tied at LOWEST score (elimination)
 * - After Round 3: Multiple players tied at HIGHEST score (winner selection)
 * 
 * MECHANISM:
 * - Server creates N cards (1 ELIMINATED, N-1 SAFE)
 * - Cards are shuffled and stacked
 * - Each tied player draws one card from the stack
 * - After all draw, cards are revealed simultaneously
 * - Player who drew ELIMINATED card is eliminated/loses
 * 
 * STATE USAGE (READ ONLY - from other PICs):
 * - MatchPlayerState.score - to determine ties
 * - MatchPlayerState.eliminated - to filter active players
 * - UserSession.state - to check connection status
 */

//==============================================================================
// BONUS ROUND TYPES
//==============================================================================

typedef enum {
    BONUS_TYPE_ELIMINATION,      // Find who to eliminate (Round 1/2)
    BONUS_TYPE_WINNER_SELECTION  // Find the winner (Round 3)
} BonusType;

typedef enum {
    BONUS_STATE_NONE = 0,
    BONUS_STATE_INITIALIZING,    // Server creating cards
    BONUS_STATE_DRAWING,         // Players drawing cards
    BONUS_STATE_REVEALING,       // Revealing results
    BONUS_STATE_APPLYING,        // Applying results
    BONUS_STATE_COMPLETED        // Done, transitioning
} BonusState;

typedef enum {
    PLAYER_BONUS_WAITING_TO_DRAW,
    PLAYER_BONUS_CARD_DRAWN,
    PLAYER_BONUS_REVEALED_SAFE,
    PLAYER_BONUS_REVEALED_ELIMINATED,
    PLAYER_BONUS_REVEALED_WINNER,
    PLAYER_BONUS_SPECTATING
} PlayerBonusState;

typedef enum {
    CARD_TYPE_SAFE = 0,
    CARD_TYPE_ELIMINATED = 1,
    CARD_TYPE_WINNER = 2
} CardType;

//==============================================================================
// API FUNCTIONS
//==============================================================================

/**
 * Handle Bonus Round commands
 * @param client_fd Client socket file descriptor
 * @param req_header Request header
 * @param payload Request payload
 */
void handle_bonus(int client_fd, MessageHeader *req_header, const char *payload);

/**
 * Check if bonus round is needed after scoring
 * Called by round handlers after scoring is complete
 * 
 * @param match_id Match identifier
 * @param after_round Round number that just ended (1, 2, or 3)
 * @return true if bonus round was triggered, false otherwise
 */
bool check_and_trigger_bonus(uint32_t match_id, int after_round);

/**
 * Handle player disconnect during Bonus Round
 * @param client_fd Client socket file descriptor
 */
void handle_bonus_disconnect(int client_fd);

/**
 * Check if bonus round is currently active for a match
 * @param match_id Match identifier
 * @return true if bonus is active
 */
bool is_bonus_active(uint32_t match_id);

/**
 * Get current bonus state
 * @param match_id Match identifier
 * @return Current bonus state
 */
BonusState get_bonus_state(uint32_t match_id);

#endif // BONUS_HANDLER_H
