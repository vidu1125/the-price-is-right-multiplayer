// Network/server/include/router.h
#ifndef ROUTER_H
#define ROUTER_H

#include <stdint.h>
#include "../../protocol/payloads.h"

// Route handlers for host features
void route_create_room(int client_fd, CreateRoomRequest* req);
void route_set_rule(int client_fd, SetRuleRequest* req);
void route_kick_member(int client_fd, KickRequest* req);
void route_delete_room(int client_fd, DeleteRoomRequest* req);
void route_start_game(int client_fd, StartGameRequest* req);

// Main router
void route_command(uint16_t command, int client_fd, void* payload, uint32_t length);

#endif // ROUTER_H
