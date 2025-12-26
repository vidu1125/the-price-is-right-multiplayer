#ifndef HISTORY_HANDLER_H
#define HISTORY_HANDLER_H

#include "protocol/protocol.h"
#include "utils/http_utils.h"

void handle_history(
    int client_fd,
    MessageHeader *req,
    const char *payload
);

#endif
