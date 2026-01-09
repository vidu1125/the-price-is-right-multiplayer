#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "protocol/protocol.h"

void forward_response(
    int client_fd,
    MessageHeader *req,
    uint16_t cmd,
    const char *payload,
    uint32_t payload_len
) {
    MessageHeader resp;
    memset(&resp, 0, sizeof(resp));

    resp.magic   = htons(MAGIC_NUMBER);
    resp.version = PROTOCOL_VERSION;
    resp.command = htons(cmd);
    resp.seq_num = htonl(req->seq_num);
    resp.length  = htonl(payload_len);

    send(client_fd, &resp, sizeof(resp), 0);
    if (payload_len > 0)
        send(client_fd, payload, payload_len, 0);
}
