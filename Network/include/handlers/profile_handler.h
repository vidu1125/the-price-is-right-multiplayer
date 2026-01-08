#ifndef PROFILE_HANDLER_H
#define PROFILE_HANDLER_H

#include <stdint.h>
#include "protocol/protocol.h"

void handle_update_profile(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

#endif // PROFILE_HANDLER_H