#ifndef ROUND3_HANDLER_H
#define ROUND3_HANDLER_H

#include "protocol/protocol.h"

/**
 * Handle Round 3 (Bonus Wheel) commands
 * @param client_fd Client socket file descriptor
 * @param req_header Request header
 * @param payload Request payload
 */
void handle_round3(int client_fd, MessageHeader *req_header, const char *payload);

/**
 * Handle player disconnect during Round 3
 * @param client_fd Client socket file descriptor
 */
void handle_round3_disconnect(int client_fd);

#endif // ROUND3_HANDLER_H
