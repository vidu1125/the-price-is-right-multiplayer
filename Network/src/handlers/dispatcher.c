#include <stdio.h>
#include <arpa/inet.h>

#include "handlers/dispatcher.h"
#include "handlers/history_handler.h"
#include "protocol/opcode.h"

void dispatch_command(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    uint16_t cmd = header->command;


    printf("[DISPATCH] cmd=0x%04x len=%u\n", cmd, header->length);

    switch (cmd) {

    case CMD_HIST:
        handle_history(client_fd, header, payload);
        break;

    default:
        printf("[DISPATCH] Unknown command: 0x%04x\n", cmd);
        // ❌ KHÔNG close socket
        break;
    }
}
