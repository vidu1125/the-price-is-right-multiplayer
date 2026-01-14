#ifndef INVITE_PLAYER_HANDLER_H
#define INVITE_PLAYER_HANDLER_H

#include "protocol/protocol.h"

void handle_invite_player(int client_fd, MessageHeader *req, const char *payload);

#endif 
