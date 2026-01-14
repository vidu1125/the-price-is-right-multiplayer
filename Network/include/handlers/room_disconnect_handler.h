#ifndef ROOM_DISCONNECT_HANDLER_H
#define ROOM_DISCONNECT_HANDLER_H

#include <stdint.h>

/**
 * Handle room cleanup when a player disconnects
 * - Removes player from RoomState
 * - Deletes from room_members table
 * - Closes room if empty
 * - Broadcasts NTF_PLAYER_LEFT if room still has players
 */
void room_handle_disconnect(int client_fd, uint32_t account_id);

#endif // ROOM_DISCONNECT_HANDLER_H
