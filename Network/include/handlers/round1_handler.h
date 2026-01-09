#ifndef ROUND1_HANDLER_H
#define ROUND1_HANDLER_H

#include "../protocol/protocol.h"

void handle_round1(
    int client_fd,
    MessageHeader *req_header,
    const char *payload
);

// Called when a client disconnects to clean up player state and notify others
void handle_round1_disconnect(int client_fd);

#endif