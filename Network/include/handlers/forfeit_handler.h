#ifndef FORFEIT_HANDLER_H
#define FORFEIT_HANDLER_H

#include <stdint.h>
#include "protocol/protocol.h"

typedef struct PACKED {
    uint32_t match_id;
} ForfeitRequest;


// Xử lý yêu cầu forfeit từ client
void handle_forfeit(int client_fd, MessageHeader *req, const char *payload);

#endif // FORFEIT_HANDLER_H
