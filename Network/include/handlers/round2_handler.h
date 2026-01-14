#ifndef ROUND2_HANDLER_H
#define ROUND2_HANDLER_H

#include "../protocol/protocol.h"

/**
 * Round 2: The Bid
 * Players bid on product prices, closest without going over wins.
 */
void handle_round2(
    int client_fd,
    MessageHeader *req_header,
    const char *payload
);

// Called when a client disconnects to clean up player state and notify others
void handle_round2_disconnect(int client_fd);

#endif
