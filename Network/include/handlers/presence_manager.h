#ifndef PRESENCE_MANAGER_H
#define PRESENCE_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "db/models/model.h"

/**
 * Presence Manager - Online Status Tracking
 * 
 * Maintains in-memory state of all online users and their presence status.
 * This is separate from the database and is RAM-only (lost on server restart).
 * 
 * CRITICAL: This must NOT block or be called inside the recv loop.
 * Use queuing if needed.
 */

// ============================================================================
// PUBLIC API
// ============================================================================

/** Initialize the presence manager */
void presence_manager_init(void);

/**
 * Register a user as online
 * 
 * Called when user successfully logs in.
 * Loads friend list from DB and broadcasts status to all online friends.
 */
void presence_register_online(
    int32_t account_id,
    int32_t client_fd,
    const char *username,
    const char *avatar
);

/**
 * Update user presence status
 * 
 * status: PRESENCE_ONLINE_IDLE or PRESENCE_PLAYING
 * room_id: non-zero if PLAYING, 0 if ONLINE_IDLE
 */
void presence_update_status(
    int32_t account_id,
    presence_status_t status,
    int32_t room_id
);

/**
 * Mark user as offline
 * 
 * Called when user disconnects or logs out.
 * Broadcasts status change to all online friends.
 */
void presence_unregister_offline(int32_t account_id);

/**
 * Get presence status of a user
 * 
 * Returns pointer to presence data, or NULL if not online
 */
const online_user_t* presence_get_user(int32_t account_id);

/**
 * Get list of online friends for a user
 * 
 * Returns array of friend_presence_t with current status
 * Caller must free the array
 */
friend_presence_t* presence_get_friends_status(
    int32_t account_id,
    int32_t *out_count
);

/**
 * Check if a user is currently online
 */
bool presence_is_online(int32_t account_id);

/**
 * Get current presence status
 */
presence_status_t presence_get_status(int32_t account_id);

/**
 * Broadcast friend status change to all online friends
 * 
 * Used internally to notify friends when status changes
 */
void presence_broadcast_status_change(
    int32_t account_id,
    presence_status_t new_status,
    int32_t room_id
);

// ============================================================================
// INTERNAL - For error recovery
// ============================================================================

/** Clean up stale online user entries */
void presence_cleanup(void);

/** Get raw online user (internal use only) */
online_user_t* presence_get_user_mut(int32_t account_id);

#endif // PRESENCE_MANAGER_H
