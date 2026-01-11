#ifndef MATCH_MANAGER_H
#define MATCH_MANAGER_H

#include <stdint.h>
#include "handlers/start_game_handler.h"

/**
 * match_manager.h - Centralized management of active matches
 * Provides APIs to create, lookup, update, and destroy match instances
 */

#define MAX_ACTIVE_MATCHES 100

//==============================================================================
// Match Manager API
//==============================================================================

/**
 * Initialize the match manager
 * Call this once at server startup
 */
void match_manager_init(void);

/**
 * Create and register a new match
 * @param match_id - Unique match identifier
 * @param room_id - Associated room ID
 * @return Pointer to created MatchState, or NULL on failure
 */
MatchState* match_create(uint32_t match_id, uint32_t room_id);

/**
 * Find an active match by match_id
 * @param match_id - Match identifier
 * @return Pointer to MatchState, or NULL if not found
 */
MatchState* match_get_by_id(uint32_t match_id);

/**
 * Find an active match by room_id
 * @param room_id - Room identifier
 * @return Pointer to MatchState, or NULL if not found
 */
MatchState* match_get_by_room(uint32_t room_id);

/**
 * Remove and destroy a match
 * @param match_id - Match identifier to destroy
 */
void match_destroy(uint32_t match_id);

/**
 * Get count of active matches (for debugging/monitoring)
 * @return Number of active matches
 */
int match_get_count(void);

/**
 * Update match status
 * @param match - Match to update
 * @param status - New status
 */
void match_set_status(MatchState *match, MatchStatus status);

#endif // MATCH_MANAGER_H
