#ifndef ROUND1_HANDLER_H
#define ROUND1_HANDLER_H

#include "../protocol/protocol.h"

void handle_round1(
    int client_fd,
    MessageHeader *req_header,
    const char *payload
);

#endif