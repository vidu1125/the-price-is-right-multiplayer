#ifndef FRIEND_HANDLER_H
#define FRIEND_HANDLER_H

#include "protocol/protocol.h"

/**
 * Friend Management Handler
 * 
 * Handles all friend-related commands:
 * - Adding friends (send friend request)
 * - Accepting/rejecting friend requests
 * - Listing friends
 * - Listing pending friend requests
 * - Removing friends
 * - Getting friend status (online/offline)
 * - Searching users to add as friends
 */

/**
 * Handle friend request send (CMD_FRIEND_ADD)
 * 
 * Expected JSON payload:
 * {
 *   "friend_email": "user@gmail.com"  // or "friend_id": 123
 * }
 * 
 * Success response (RES_FRIEND_ADDED):
 * {
 *   "success": true,
 *   "request_id": 456,
 *   "message": "Friend request sent"
 * }
 * 
 * Error responses:
 * - ERR_BAD_REQUEST: Invalid payload
 * - ERR_NOT_LOGGED_IN: User not authenticated
 * - ERR_FRIEND_NOT_FOUND: Friend user not found
 * - ERR_CONFLICT: Friend request already exists
 */
void handle_friend_add(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle accept friend request (CMD_FRIEND_ACCEPT)
 * 
 * Expected JSON payload:
 * {
 *   "request_id": 456
 * }
 * 
 * Success response (RES_FRIEND_REQUEST_ACCEPTED):
 * {
 *   "success": true,
 *   "friend_id": 123,
 *   "message": "Friend request accepted"
 * }
 */
void handle_friend_accept(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle reject friend request (CMD_FRIEND_REJECT)
 * 
 * Expected JSON payload:
 * {
 *   "request_id": 456
 * }
 * 
 * Success response (RES_FRIEND_REQUEST_REJECTED):
 * {
 *   "success": true,
 *   "message": "Friend request rejected"
 * }
 */
void handle_friend_reject(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle remove friend (CMD_FRIEND_REMOVE)
 * 
 * Expected JSON payload:
 * {
 *   "friend_id": 123
 * }
 * 
 * Success response (RES_FRIEND_REMOVED):
 * {
 *   "success": true,
 *   "message": "Friend removed"
 * }
 */
void handle_friend_remove(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle get friend list (CMD_FRIEND_LIST)
 * 
 * Expected JSON payload: {} (empty or no payload)
 * 
 * Success response (RES_FRIEND_LIST):
 * {
 *   "success": true,
 *   "friends": [
 *     {
 *       "id": 123,
 *       "name": "John Doe",
 *       "email": "john@gmail.com",
 *       "avatar": "url",
 *       "online": true,
 *       "in_game": false
 *     },
 *     ...
 *   ]
 * }
 */
void handle_friend_list(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle get pending friend requests (CMD_FRIEND_REQUESTS)
 * 
 * Expected JSON payload: {} (empty or no payload)
 * 
 * Success response (RES_FRIEND_REQUESTS):
 * {
 *   "success": true,
 *   "requests": [
 *     {
 *       "id": 456,
 *       "sender_id": 789,
 *       "sender_name": "Jane Doe",
 *       "sender_email": "jane@gmail.com",
 *       "sender_avatar": "url",
 *       "created_at": "2026-01-12T10:30:00Z"
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

/**
 * Handle search users to add as friends (CMD_SEARCH_USER)
 * 
 * Expected JSON payload:
 * {
 *   "query": "john"  // search by name or email
 * }
 * 
 * Success response (RES_SUCCESS):
 * {
 *   "success": true,
 *   "users": [
 *     {
 *       "id": 123,
 *       "name": "John Doe",
 *       "email": "john@gmail.com",
 *       "avatar": "url",
 *       "is_friend": false,
 *       "request_pending": false
 *     },
 *     ...
 *   ]
 * }
 */
void handle_search_user(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Main friend command dispatcher
 * Routes friend-related commands to appropriate handlers
 */
void handle_friend(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

#endif // FRIEND_HANDLER_H
