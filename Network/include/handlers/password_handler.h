#ifndef PASSWORD_HANDLER_H
#define PASSWORD_HANDLER_H

#include "protocol/protocol.h"

/*
 * CMD_CHANGE_PASSWORD (0x0106)
 */
typedef struct PACKED {
    char old_password[64];
    char new_password[64];
} ChangePasswordRequest;

// Response uses JSON (RES_PASSWORD_CHANGED)

void handle_change_password(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

#endif
