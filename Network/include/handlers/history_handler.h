#ifndef HISTORY_HANDLER_H
#define HISTORY_HANDLER_H

#include "protocol/protocol.h"

void handle_history(
    int client_fd,
    MessageHeader *req_header,
    const char *payload,
    int32_t account_id
);

#endif
