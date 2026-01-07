#include "handlers/auth_guard.h"
#include "handlers/session_context.h"
#include "db/repo/session_repo.h"
#include "protocol/opcode.h"
#include <string.h>

bool require_auth(int client_fd, MessageHeader *req) {
    const char *sid = get_client_session(client_fd);
    if (!sid) {
        const char *msg = "Not logged in";
        forward_response(client_fd, req, ERR_NOT_LOGGED_IN, msg, strlen(msg));
        return false;
    }

    // Validate session still active in DB/cache
    if (!session_is_valid(sid)) {
        clear_client_session(client_fd);
        const char *msg = "Session invalid";
        forward_response(client_fd, req, ERR_NOT_LOGGED_IN, msg, strlen(msg));
        return false;
    }

    return true;
}
