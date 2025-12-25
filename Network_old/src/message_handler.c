#include <stdio.h>
#include <string.h>
//#include "../../include/message_handler.h"
#include "../include/protocol/protocol.h"
#include "../include/server.h"

//==============================================================================
// COMMON HANDLER UTILITIES
//==============================================================================

int validate_client_state(int client_idx, ConnectionState expected_state) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->state != expected_state) {
        const char *state_names[] = {
            "UNAUTHENTICATED",
            "AUTHENTICATED", 
            "IN_ROOM",
            "PLAYING"
        };
        
        printf("Client %d: Invalid state. Expected %s, got %s\n",
               client_idx,
               state_names[expected_state],
               state_names[client->state]);
        
        return 0;
    }
    
    return 1;
}

int require_authentication(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->state == STATE_UNAUTHENTICATED) {
        send_response(client->sockfd, ERR_NOT_LOGGED_IN, 
                     "Authentication required", 23);
        return 0;
    }
    
    return 1;
}

int require_room(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->room_id < 0 || client->room_id >= MAX_ROOMS) {
        send_response(client->sockfd, ERR_BAD_REQUEST, 
                     "Not in a room", 13);
        return 0;
    }
    
    return 1;
}

int require_playing(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->state != STATE_PLAYING) {
        send_response(client->sockfd, ERR_BAD_REQUEST, 
                     "Not in game", 11);
        return 0;
    }
    
    return 1;
}

int require_host(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    
    if (!require_room(client_idx)) {
        return 0;
    }
    
    GameRoom *room = &rooms[client->room_id];
    
    if (room->host_client_idx != client_idx) {
        send_response(client->sockfd, ERR_NOT_HOST, 
                     "Not room host", 13);
        return 0;
    }
    
    return 1;
}

void send_error(int client_idx, uint16_t error_code, const char *message) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->sockfd != -1) {
        send_response(client->sockfd, error_code, message, strlen(message));
    }
    
    printf("Client %d: Error %d - %s\n", client_idx, error_code, message);
}

void send_success(int client_idx, const char *message) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->sockfd != -1) {
        send_response(client->sockfd, RES_SUCCESS, message, strlen(message));
    }
}

int validate_payload_size(int client_idx, uint32_t actual_size, 
                          uint32_t expected_size, const char *payload_name) {
    if (actual_size < expected_size) {
        printf("Client %d: Invalid %s payload size: %u (expected >= %u)\n",
               client_idx, payload_name, actual_size, expected_size);
        
        send_error(client_idx, ERR_BAD_REQUEST, "Invalid payload size");
        return 0;
    }
    
    return 1;
}

void log_command(int client_idx, uint16_t command, const char *description) {
    ClientConnection *client = &clients[client_idx];
    
    printf("[Client %d | User: %s | State: %d] Command 0x%04X: %s\n",
           client_idx,
           client->display_name[0] ? client->display_name : "anonymous",
           client->state,
           command,
           description);
}

void log_error(int client_idx, const char *context, const char *error) {
    printf("[Client %d] ERROR in %s: %s\n", client_idx, context, error);
}

void log_info(int client_idx, const char *message) {
    printf("[Client %d] INFO: %s\n", client_idx, message);
}
