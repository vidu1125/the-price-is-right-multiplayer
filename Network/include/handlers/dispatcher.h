#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "protocol/protocol.h"

/**
 * Dispatch parsed command to application handlers.
 *
 * - Implemented by handler layer
 * - Called by socket_server
 * - Must NOT block
 */
void dispatch_command(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

#endif // DISPATCHER_H
