#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "handlers/history_handler.h"
#include "protocol/protocol.h"
#include "protocol/opcode.h"
/*
 * CMD_HIST
 * - no payload
 * - respond with CMD_HIST_RESP
 */
void handle_history(
    int client_fd,
    MessageHeader *req_header,
    const char *payload
) {
    (void)payload;

    if (req_header->length != 0) {
        printf("[HIST] payload should be empty\n");
        return;
    }

    printf("[HIST] handle history request\n");

    const char *msg = "history not implemented yet";
    uint32_t msg_len = (uint32_t)strlen(msg);

    MessageHeader resp;
    memset(&resp, 0, sizeof(resp));   // ✅ RẤT QUAN TRỌNG

    resp.magic    = htons(MAGIC_NUMBER);
    resp.version  = PROTOCOL_VERSION;
    resp.flags    = 0;
    resp.command  = htons(CMD_HIST);   // ✅ RESP opcode
    resp.reserved = 0;
    resp.seq_num  = htonl(req_header->seq_num);
    resp.length   = htonl(msg_len);

    // Send header
    if (send(client_fd, &resp, sizeof(resp), 0) != sizeof(resp)) {
        perror("[HIST] send header failed");
        return;
    }

    // Send payload
    if (msg_len > 0) {
        if (send(client_fd, msg, msg_len, 0) != (ssize_t)msg_len) {
            perror("[HIST] send payload failed");
            return;
        }
    }

    printf("[HIST] response sent\n");
}
