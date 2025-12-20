#include <stdio.h>
#include <string.h>

#include "../../include/message_handler.h"
#include "../../include/protocol.h"
#include "../../include/server.h"
#include "../../include/room_manager.h"

//==============================================================================
// CHAT HANDLER
//==============================================================================

void handle_chat(int client_idx, char *payload, uint32_t length) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->room_id < 0 || client->room_id >= MAX_ROOMS) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Not in room", 11);
        return;
    }
    
    if (length == 0 || length > 256) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Invalid message", 15);
        return;
    }
    
    char msg[512];
    snprintf(msg, sizeof(msg), "[%s]: %.*s", client->display_name, (int)length, payload);
    
    broadcast_to_room(client->room_id, NTF_CHAT_MSG, msg, strlen(msg));
    send_response(client->sockfd, RES_SUCCESS, "Message sent", 12);
}

//==============================================================================
// HISTORY HANDLER
//==============================================================================

void handle_history(int client_idx) {
    // Stub: Would query match_history table
    const char *stub_data = "{\"matches\":[{\"id\":1,\"result\":\"win\",\"score\":1500}]}";
    send_response(clients[client_idx].sockfd, RES_SUCCESS, stub_data, strlen(stub_data));
    printf("Client %d requested history\n", client_idx);
}

//==============================================================================
// LEADERBOARD HANDLER
//==============================================================================

void handle_leaderboard(int client_idx) {
    // Stub: Would query users table sorted by balance
    const char *stub_data = "{\"top\":[{\"name\":\"Player1\",\"score\":5000}]}";
    send_response(clients[client_idx].sockfd, RES_SUCCESS, stub_data, strlen(stub_data));
    printf("Client %d requested leaderboard\n", client_idx);
}