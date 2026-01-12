#ifndef ROOM_HANDLER_H
#define ROOM_HANDLER_H

#include "protocol/protocol.h"

/**
 * Handle CMD_CREATE_ROOM (0x0200)
 * Host creates a new game room
 */
void handle_create_room(
    int client_fd,
    MessageHeader *req,
    const char *payload
);

/**
 * Handle CMD_CLOSE_ROOM (0x0207)
 * Host closes the room (kicks all members)
 */
void handle_close_room(
    int client_fd,
    MessageHeader *req,
    const char *payload
);

/**
 * Handle CMD_SET_RULE (0x0206)
 * Host changes room settings
 */
void handle_set_rules(
    int client_fd,
    MessageHeader *req,
    const char *payload
);

/**
 * Handle CMD_KICK (0x0204)
 * Host kicks a specific member
 */
void handle_kick_member(
    int client_fd,
    MessageHeader *req,
    const char *payload
);

/**
 * Handle CMD_LEAVE_ROOM (0x0202)
 * Member leaves the room
 */
void handle_leave_room(
    int client_fd,
    MessageHeader *req,
    const char *payload
);

/**
 * Handle CMD_GET_ROOM_LIST (0x0208)
 * Get list of waiting rooms
 */
void handle_get_room_list(
    int client_fd,
    MessageHeader *req,
    const char *payload
);

#endif // ROOM_HANDLER_H
