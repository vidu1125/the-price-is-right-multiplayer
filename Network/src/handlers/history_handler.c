#include <string.h>
#include "handlers/history_handler.h"
#include "protocol/opcode.h"
#include "db/repo/question_repo.h"
#include "protocol/protocol.h"

void handle_history(
    int client_fd,
    MessageHeader *req_header,
    const char *payload
) {
    (void)payload;

    char resp_buf[1024];

    // 🔁 Replace backend HTTP with repo call
    int resp_len = history_repo_get(
        resp_buf,
        sizeof(resp_buf)
    );

    if (resp_len <= 0) {
        const char *msg = "{\"message\":\"failed to fetch history\"}";
        resp_len = strlen(msg);
        memcpy(resp_buf, msg, resp_len);
    }

    forward_response(
        client_fd,
        req_header,
        CMD_HIST,
        resp_buf,
        (uint32_t)resp_len
    );
}
