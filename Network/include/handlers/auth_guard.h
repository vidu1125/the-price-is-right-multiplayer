#ifndef AUTH_GUARD_H
#define AUTH_GUARD_H

#include <stdbool.h>
#include "protocol/protocol.h"

// Validate that the current connection has a bound, valid session.
// Returns true if allowed to proceed; otherwise sends ERR_NOT_LOGGED_IN and returns false.
bool require_auth(int client_fd, MessageHeader *req);

#endif // AUTH_GUARD_H