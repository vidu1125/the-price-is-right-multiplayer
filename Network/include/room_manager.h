#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include "server.h"

//==============================================================================
// ROOM MANAGEMENT FUNCTIONS
//==============================================================================

void init_rooms(void);
int find_empty_room(void);
int find_room_by_id(uint32_t room_id);
int create_room(int host_client_idx, uint8_t max_players, GameMode mode);
int add_player_to_room(int room_idx, int client_idx);
void remove_player_from_room(int room_idx, int client_idx);
void broadcast_to_room(int room_idx, uint16_t command, const char *payload, uint32_t length);

#endif // ROOM_MANAGER_H