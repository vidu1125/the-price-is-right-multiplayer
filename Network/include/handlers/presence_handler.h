#ifndef PRESENCE_HANDLER_H
#define PRESENCE_HANDLER_H

#include "protocol/protocol.h"

/**
 * Presence & Status Handler
 * 
 * Handles user presence status updates:
 * - CMD_STATUS_UPDATE (0x050A): Update user presence status
 * - CMD_GET_FRIEND_STATUS (0x050B): Get specific friend's status
 */

/**
 * Handle status update request
 * 
 * Expected JSON payload:
 * {
 *   "status": 1 or 2,  // 1=PRESENCE_ONLINE_IDLE, 2=PRESENCE_PLAYING
 *   "room_id": 123     // present only if status=PLAYING
 * }
 * 
 * Success response (RES_STATUS_UPDATED):
 * {
 *   "success": true,
 *   "status": 2,
 *   "message": "Status updated"
 * }
 * 
 * This will broadcast status changes to all online friends via NTF_FRIEND_STATUS.
 * 
 * Error responses:
 * - ERR_NOT_LOGGED_IN: User not authenticated
 * - ERR_BAD_REQUEST: Invalid payload or status value
 */
void handle_status_update(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle get friend status request
 * 
 * Expected JSON payload:
 * {
 *   "friend_id": 456
 * }
 * 
 * Success response:
 * {
 *   "success": true,
 *   "friend_id": 456,
 *   "status": 1,  // 0=OFFLINE, 1=ONLINE_IDLE, 2=PLAYING
 *   "room_id": 0
 * }
 * 
 * Error responses:
 * - ERR_NOT_LOGGED_IN
 * - ERR_BAD_REQUEST: Missing friend_id
 */
void handle_get_friend_status(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

#endif // PRESENCE_HANDLER_H
