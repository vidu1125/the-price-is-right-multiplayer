#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "../../include/message_handler.h"
#include "../../include/protocol.h"
#include "../../include/server.h"
#include "../../include/room_manager.h"

//==============================================================================
// CREATE ROOM HANDLER
//==============================================================================

void handle_create_room(int client_idx, char *payload, uint32_t length) {
    ClientConnection *client = &clients[client_idx];
    
    if (length < sizeof(CreateRoomPayload)) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Invalid payload", 15);
        return;
    }
    
    CreateRoomPayload *req = (CreateRoomPayload *)payload;
    
    int room_idx = create_room(client_idx, req->max_players, (GameMode)req->game_mode);
    if (room_idx < 0) {
        send_response(client->sockfd, ERR_SERVER_ERROR, "Failed to create room", 21);
        return;
    }
    
    client->state = STATE_IN_ROOM;
    
    RoomInfoPayload info;
    info.room_id = htonl(rooms[room_idx].room_id);
    info.player_count = 1;
    info.max_players = req->max_players;
    info.game_mode = req->game_mode;
    info.is_playing = 0;
    strncpy(info.host_name, client->display_name, 31);
    
    send_response(client->sockfd, RES_SUCCESS, (char *)&info, sizeof(info));
    
    printf("Client %d created room %u\n", client_idx, rooms[room_idx].room_id);
}

//==============================================================================
// JOIN ROOM HANDLER
//==============================================================================

void handle_join_room(int client_idx, char *payload, uint32_t length) {
    ClientConnection *client = &clients[client_idx];
    
    if (length < sizeof(JoinRoomPayload)) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Invalid payload", 15);
        return;
    }
    
    JoinRoomPayload *req = (JoinRoomPayload *)payload;
    uint32_t room_id = ntohl(req->room_id);
    
    int room_idx = find_room_by_id(room_id);
    if (room_idx < 0) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Room not found", 14);
        return;
    }
    
    GameRoom *room = &rooms[room_idx];
    
    if (room->is_playing) {
        send_response(client->sockfd, ERR_GAME_STARTED, "Game already started", 20);
        return;
    }
    
    if (room->player_count >= room->max_players) {
        send_response(client->sockfd, ERR_ROOM_FULL, "Room is full", 12);
        return;
    }
    
    if (add_player_to_room(room_idx, client_idx) < 0) {
        send_response(client->sockfd, ERR_SERVER_ERROR, "Failed to join", 14);
        return;
    }
    
    client->state = STATE_IN_ROOM;
    
    send_response(client->sockfd, RES_ROOM_JOINED, "Joined room", 11);
    
    // Notify other players
    char msg[128];
    snprintf(msg, sizeof(msg), "%s joined the room", client->display_name);
    broadcast_to_room(room_idx, NTF_PLAYER_JOINED, msg, strlen(msg));
    
    printf("Client %d joined room %u\n", client_idx, room_id);
}

//==============================================================================
// LEAVE ROOM HANDLER
//==============================================================================

void handle_leave_room(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->room_id < 0 || client->room_id >= MAX_ROOMS) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Not in a room", 13);
        return;
    }
    
    int room_idx = client->room_id;
    
    // Notify others before leaving
    char msg[128];
    snprintf(msg, sizeof(msg), "%s left the room", client->display_name);
    broadcast_to_room(room_idx, NTF_PLAYER_LEFT, msg, strlen(msg));
    
    remove_player_from_room(room_idx, client_idx);
    
    client->state = STATE_AUTHENTICATED;
    send_response(client->sockfd, RES_SUCCESS, "Left room", 9);
    
    printf("Client %d left room\n", client_idx);
}

//==============================================================================
// READY HANDLER
//==============================================================================

void handle_ready(int client_idx, char *payload, uint32_t length) {
    (void)payload;  
    (void)length;  // 
    ClientConnection *client = &clients[client_idx];
    
    if (client->room_id < 0 || client->room_id >= MAX_ROOMS) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Not in a room", 13);
        return;
    }
    
    GameRoom *room = &rooms[client->room_id];
    
    // Find player and toggle ready status
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].client_idx == client_idx) {
            room->players[i].is_ready = !room->players[i].is_ready;
            
            char msg[64];
            snprintf(msg, sizeof(msg), "%s is %s", 
                    client->display_name,
                    room->players[i].is_ready ? "ready" : "not ready");
            
            broadcast_to_room(client->room_id, NTF_PLAYER_LIST, msg, strlen(msg));
            send_response(client->sockfd, RES_SUCCESS, "Ready status updated", 20);
            
            printf("Client %d ready status: %d\n", client_idx, room->players[i].is_ready);
            return;
        }
    }
}

//==============================================================================
// KICK HANDLER
//==============================================================================

void handle_kick(int client_idx, char *payload, uint32_t length) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->room_id < 0 || client->room_id >= MAX_ROOMS) {
        send_response(client->sockfd, ERR_NOT_HOST, "Not in room", 11);
        return;
    }
    
    GameRoom *room = &rooms[client->room_id];
    
    if (room->host_client_idx != client_idx) {
        send_response(client->sockfd, ERR_NOT_HOST, "Not room host", 13);
        return;
    }
    
    if (length < sizeof(uint32_t)) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Invalid payload", 15);
        return;
    }
    
    uint32_t target_user_id = ntohl(*(uint32_t *)payload);
    
    // Find and kick target player
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].user_id == target_user_id) {
            int target_idx = room->players[i].client_idx;
            
            send_response(clients[target_idx].sockfd, ERR_SERVER_ERROR, "Kicked from room", 16);
            remove_player_from_room(client->room_id, target_idx);
            
            send_response(client->sockfd, RES_SUCCESS, "Player kicked", 13);
            printf("Client %d kicked player %u\n", client_idx, target_user_id);
            return;
        }
    }
    
    send_response(client->sockfd, ERR_BAD_REQUEST, "Player not found", 16);
}