#ifndef FRIEND_REPO_H
#define FRIEND_REPO_H

#include "db/core/db_error.h"
#include "db/models/model.h"

/**
 * Friend Management Repository
 * 
 * Handles database operations for friendships and friend requests.
 * All operations are blocking and must NOT be called inside the realtime loop.
 */

// ============================================================================
// FRIEND REQUESTS
// ============================================================================

/**
 * Create or update a friend request
 * 
 * Returns DB_ERR_CONFLICT if request already exists
 */
db_error_t friend_request_create(
    int32_t sender_id,
    int32_t receiver_id,
    friend_request_t **out_request
);

/**
 * Get pending friend requests for a user (as receiver)
 * 
 * Returns array of friend_request_t with sender details
 */
db_error_t friend_request_list(
    int32_t receiver_id,
    friend_request_t ***out_requests,
    int32_t *out_count
);

/**
 * Accept a friend request
 * 
 * - Updates friend_requests status to ACCEPTED
 * - Creates bidirectional friendship (both directions)
 * - Error handling:
 *   - ERR_NOT_FOUND if request doesn't exist
 *   - ERR_INVALID if request is not PENDING
 */
db_error_t friend_request_accept(
    int32_t request_id,
    friend_request_t **out_request
);

/**
 * Reject a friend request
 * 
 * - Updates friend_requests status to REJECTED
 */
db_error_t friend_request_reject(
    int32_t request_id,
    friend_request_t **out_request
);

// ============================================================================
// FRIENDSHIPS
// ============================================================================

/**
 * Get list of friend IDs for a user
 * 
 * Returns array of account_ids that are friends with user_id
 * Error handling:
 * - Returns empty array if user has no friends
 */
db_error_t friend_list_get_ids(
    int32_t user_id,
    int32_t **out_friend_ids,
    int32_t *out_count
);

/**
 * Get full friend details with profile info
 * 
 * Returns array of friend_t structs with profile data
 */
db_error_t friend_list_get_full(
    int32_t user_id,
    friend_t ***out_friends,
    int32_t *out_count
);

/**
 * Check if two users are friends
 */
db_error_t friend_check_relationship(
    int32_t user_id_1,
    int32_t user_id_2,
    bool *out_are_friends
);

/**
 * Remove a friendship (both directions)
 * 
 * - Deletes both (user_id_1, user_id_2) and (user_id_2, user_id_1)
 * - Error handling:
 *   - ERR_NOT_FOUND if friendship doesn't exist
 */
db_error_t friend_remove(
    int32_t user_id_1,
    int32_t user_id_2
);

/**
 * Block a user (optional feature for future)
 * 
 * Currently not implemented - may extend friend_requests status
 */
db_error_t friend_block(
    int32_t user_id,
    int32_t blocked_user_id
);

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

void friend_request_free(friend_request_t *req);
void friend_request_free_array(friend_request_t **reqs, int32_t count);

void friend_free(friend_t *f);
void friend_free_array(friend_t **friends, int32_t count);

#endif // FRIEND_REPO_H
