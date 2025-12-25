#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../include/room_manager.h"
#include "../include/protocol/protocol.h"
//==============================================================================
// ROOM INITIALIZATION
//==============================================================================

void init_rooms() {
    for (int i = 0; i < MAX_ROOMS; i++) {
        rooms[i].room_id = 0;
        rooms[i].player_count = 0;
        rooms[i].is_playing = 0;
    }
}

//==============================================================================
// ROOM FINDING
//==============================================================================

int find_empty_room() {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].room_id == 0) {
            return i;
        }
    }
    return -1;
}

int find_room_by_id(uint32_t room_id) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].room_id == room_id) {
            return i;
        }
    }
    return -1;
}

//==============================================================================
// ROOM CREATION
//==============================================================================

int create_room(int host_client_idx, uint8_t max_players, GameMode mode) {
    int room_idx = find_empty_room();
    if (room_idx == -1) {
        return -1;
    }
    
    GameRoom *room = &rooms[room_idx];
    room->room_id = (uint32_t)(time(NULL) ^ room_idx);
    room->host_client_idx = host_client_idx;
    room->player_count = 1;
    room->max_players = max_players;
    room->game_mode = mode;
    room->is_playing = 0;
    room->current_round = ROUND_QUIZ;
    
    // Add host as first player
    Player *player = &room->players[0];
    player->client_idx = host_client_idx;
    player->user_id = clients[host_client_idx].user_id;
    strncpy(player->display_name, clients[host_client_idx].display_name, 31);
    player->score = 0;
    player->is_ready = 1;
    player->is_eliminated = 0;
    
    clients[host_client_idx].room_id = room_idx;
    
    return room_idx;
}

//==============================================================================
// PLAYER MANAGEMENT
//==============================================================================

int add_player_to_room(int room_idx, int client_idx) {
    GameRoom *room = &rooms[room_idx];
    
    if (room->player_count >= room->max_players) {
        return -1;
    }
    
    Player *player = &room->players[room->player_count];
    player->client_idx = client_idx;
    player->user_id = clients[client_idx].user_id;
    strncpy(player->display_name, clients[client_idx].display_name, 31);
    player->score = 0;
    player->is_ready = 0;
    player->is_eliminated = 0;
    
    room->player_count++;
    clients[client_idx].room_id = room_idx;
    
    return 0;
}

void remove_player_from_room(int room_idx, int client_idx) {
    GameRoom *room = &rooms[room_idx];
    
    int player_idx = -1;
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].client_idx == client_idx) {
            player_idx = i;
            break;
        }
    }
    
    if (player_idx == -1) return;
    
    // Shift players
    for (int i = player_idx; i < room->player_count - 1; i++) {
        room->players[i] = room->players[i + 1];
    }
    room->player_count--;
    
    clients[client_idx].room_id = -1;
    
    // Destroy room if empty
    if (room->player_count == 0) {
        room->room_id = 0;
        room->is_playing = 0;
    }
    // Assign new host if needed
    else if (room->host_client_idx == client_idx) {
        room->host_client_idx = room->players[0].client_idx;
    }
}

//==============================================================================
// BROADCAST
//==============================================================================

void broadcast_to_room(int room_idx, uint16_t command, const char *payload, uint32_t length) {
    GameRoom *room = &rooms[room_idx];
    
    for (int i = 0; i < room->player_count; i++) {
        int client_idx = room->players[i].client_idx;
        if (clients[client_idx].sockfd != -1) {
            send_response(clients[client_idx].sockfd, command, payload, length);
        }
    }
}