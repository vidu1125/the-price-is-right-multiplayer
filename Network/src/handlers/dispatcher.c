#include <stdio.h>
#include <arpa/inet.h>

#include "handlers/dispatcher.h"
#include "handlers/history_handler.h"
#include "handlers/room_handler.h"
#include "handlers/profile_handler.h"
#include "handlers/round1_handler.h"
#include "handlers/start_game_handler.h"
#include "handlers/session_context.h"
#include "handlers/auth_guard.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"

#include <string.h>

void dispatch_command(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    int32_t account_id = 0;   
    uint16_t cmd = header->command;

    printf("[DISPATCH] Receiving: cmd=0x%04x len=%u\n", cmd, header->length);
    bool is_auth_cmd = (cmd == CMD_LOGIN_REQ || cmd == CMD_REGISTER_REQ || cmd == CMD_RECONNECT || cmd == CMD_LOGOUT_REQ);
    
    if (!is_auth_cmd) {
        if (!require_auth(client_fd, header)) return;
        
        account_id = get_client_account(client_fd);
        if (account_id == 0) {
            printf("[DISPATCH] Error: Authenticated query but account_id is 0\n");
            return;
        }
        else {
            printf("[DISPATCH] Authenticated query, account_id: %d\n", account_id);
        }
    }

    switch (cmd) {

    // Auth
    case CMD_LOGIN_REQ:
        handle_login(client_fd, header, payload);
        break;
    case CMD_REGISTER_REQ:
        handle_register(client_fd, header, payload);
        break;
    case CMD_LOGOUT_REQ:
        handle_logout(client_fd, header, payload);
        break;
    case CMD_RECONNECT:
        handle_reconnect(client_fd, header, payload);
        break;

    // Profile / Player
    case CMD_UPDATE_PROFILE:
        handle_update_profile(client_fd, header, payload);
        break;
    case CMD_GET_PROFILE:
        handle_get_profile(client_fd, header, payload);
        break;

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
    case CMD_GET_ROOM_LIST:
        handle_get_room_list(client_fd, header, payload);
        break;

    // Match Management
    case CMD_START_GAME:
        handle_start_game(client_fd, header, payload);
        break;

    // History
    case CMD_HIST:
        printf("[DISPATCH] Parsing to historyHandler\n");        
        handle_history(
            client_fd,
            header,
            payload,
            account_id
        );        
        break;
    case OP_C2S_ROUND1_READY:
    case OP_C2S_ROUND1_GET_QUESTION:
    case OP_C2S_ROUND1_ANSWER:
    case OP_C2S_ROUND1_END:
    case OP_C2S_ROUND1_PLAYER_READY:
    case OP_C2S_ROUND1_FINISHED:
        handle_round1(client_fd, header, payload);
        break;

    case CMD_REPLAY:
        printf("[DISPATCH] Parsing to replayHandler\n");       
        handle_replay(client_fd, header, payload, account_id);
        break;

    default:
        printf("[DISPATCH] Unknown command: 0x%04x\n", cmd);
        break;
    }
}
