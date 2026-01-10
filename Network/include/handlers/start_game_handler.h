#ifndef START_GAME_HANDLER_H
#define START_GAME_HANDLER_H

#include <stdint.h>
#include "protocol/protocol.h"

typedef struct PACKED {
    uint32_t room_id;
} StartGameRequest;


void handle_start_game(
    int client_fd,
    MessageHeader *req,
    const char *payload
);

#endif // START_GAME_HANDLER_H
