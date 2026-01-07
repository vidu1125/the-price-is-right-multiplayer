#include <stdio.h>
#include <arpa/inet.h>

#include "handlers/dispatcher.h"
#include "handlers/history_handler.h"
#include "handlers/room_handler.h"
#include "handlers/match_handler.h"
#include "protocol/opcode.h"

void dispatch_command(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    int32_t account_id = 1;   
    uint16_t cmd = header->command;

    printf("[DISPATCH] cmd=0x%04x len=%u\n", cmd, header->length);

    switch (cmd) {

    // Room Management
    case CMD_CREATE_ROOM:
        handle_create_room(client_fd, header, payload);
        break;
    case CMD_LEAVE_ROOM:
        handle_leave_room(client_fd, header, payload);
        break;
    case CMD_CLOSE_ROOM:
        handle_close_room(client_fd, header, payload);
        break;
    case CMD_SET_RULE:
        handle_set_rules(client_fd, header, payload);
        break;
    case CMD_KICK:
        handle_kick_member(client_fd, header, payload);
        break;
    case CMD_GET_ROOM_STATE:
        handle_get_room_state(client_fd, header, payload);
        break;

    // Match Management
    case CMD_START_GAME:
        handle_start_game(client_fd, header, payload);
        break;

    // History
    case CMD_HIST:
        handle_history(
            client_fd,
            header,
            payload,
            account_id    // 🔹 truyền từ dispatcher
        );        
        break;

    default:
        printf("[DISPATCH] Unknown command: 0x%04x\n", cmd);
        break;
    }
}
