#ifndef SOCIAL_HANDLER_H
#define SOCIAL_HANDLER_H

#include "protocol/protocol.h"

/**
 * Social Features Handler
 * 
 * Handles friend management:
 * - CMD_FRIEND_ADD (0x0501): Send friend request
 * - CMD_FRIEND_ACCEPT (0x0505): Accept friend request
 * - CMD_FRIEND_REJECT (0x0506): Reject friend request
 * - CMD_FRIEND_REMOVE (0x0507): Remove friend
 * - CMD_FRIEND_LIST (0x0508): Get list of friends with status
 * - CMD_FRIEND_REQUESTS (0x0509): Get pending friend requests
 */

/**
 * Handle friend add request
 * 
 * Expected JSON payload:
 * {
 *   "target_username": "friend_name"  // or target_id for optimization
 * }
 * 
 * Success response (RES_FRIEND_ADDED):
 * {
 *   "success": true,
 *   "request_id": 123,
 *   "receiver_id": 456,
 *   "message": "Friend request sent"
 * }
 * 
 * Error responses:
 * - ERR_NOT_LOGGED_IN: User not authenticated
 * - ERR_BAD_REQUEST: Invalid payload
 * - ERR_FRIEND_NOT_FOUND: Target user doesn't exist
 * - ERR_ALREADY_FRIENDS: Already friends
 * - ERR_FRIEND_REQ_DUPLICATE: Request already pending
 * - ERR_SELF_ADD: Cannot add self
 * 
 * Notifications:
 * - NTF_FRIEND_REQUEST sent to target if online
 */
void handle_friend_add(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle friend request acceptance
 * 
 * Expected JSON payload:
 * {
 *   "request_id": 123
 * }
 * 
 * Success response (RES_FRIEND_REQUEST_ACCEPTED):
 * {
 *   "success": true,
 *   "friend_id": 456,
 *   "message": "Friend request accepted"
 * }
 * 
 * Error responses:
 * - ERR_NOT_LOGGED_IN
 * - ERR_BAD_REQUEST
 * - ERR_FRIEND_REQ_NOT_FOUND: Request doesn't exist
 * 
 * Notifications:
 * - NTF_FRIEND_ACCEPTED sent to sender
 */
void handle_friend_accept(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle friend request rejection
 * 
 * Expected JSON payload:
 * {
 *   "request_id": 123
 * }
 * 
 * Success response (RES_FRIEND_REQUEST_REJECTED)
 */
void handle_friend_reject(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle friend removal
 * 
 * Expected JSON payload:
 * {
 *   "friend_id": 456
 * }
 * 
 * Success response (RES_FRIEND_REMOVED)
 */
void handle_friend_remove(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle get friends list with presence status
 * 
 * Expected JSON payload: {} (empty or null)
 * 
 * Success response (RES_FRIEND_LIST):
 * {
 *   "success": true,
 *   "friends": [
 *     {
 *       "friend_id": 456,
 *       "friend_name": "Alice",
 *       "friend_avatar": "url",
 *       "status": 1,  // 0=OFFLINE, 1=ONLINE_IDLE, 2=PLAYING
 *       "room_id": 0   // present if PLAYING
 *     },
 *     ...
 *   ]
 * }
 * 
 * Error responses:
 * - ERR_NOT_LOGGED_IN
 */
void handle_friend_list(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle get pending friend requests
 * 
 * Expected JSON payload: {} (empty or null)
 * 
 * Success response (RES_FRIEND_REQUESTS):
 * {
 *   "success": true,
 *   "requests": [
 *     {
 *       "request_id": 123,
 *       "sender_id": 789,
 *       "sender_name": "Bob",
 *       "sender_avatar": "url",
 *       "created_at": "2025-01-10T12:34:56Z"
 *     },
 *     ...
 *   ]
 * }
 */
void handle_friend_requests(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

#endif // SOCIAL_HANDLER_H
