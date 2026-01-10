#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "handlers/start_game_handler.h"
#include "transport/socket_server.h"
#include "protocol/opcode.h"

void handle_start_game(int client_fd, MessageHeader *req, const char *payload) {
    if (req->length != sizeof(StartGameRequest)) {
        printf("[HANDLER] <startgame> Invalid payload length: %u\n", req->length);
        return;
    }

    StartGameRequest *request = (StartGameRequest *)payload;
    uint32_t room_id = ntohl(request->room_id);

    printf("[HANDLER] <startgame> Request received for Room ID: %u\n", room_id);

    // TODO: Validate room state
    // Check if room exists
    // Check if requester is host
    // Check if room is full/ready
    
    if (1) {
        printf("[HANDLER] <startgame> Validation placeholder passed (TODO: check room state)\n");
        
        // Logic to start game would go here
        // e.g., set room state to PLAYING, broadcast game start to members
    }
}
