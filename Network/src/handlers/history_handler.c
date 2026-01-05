#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // free

#include "handlers/history_handler.h"
#include "protocol/opcode.h"
#include "db/repo/question_repo.h"
#include "protocol/protocol.h"
#include <cjson/cJSON.h>




void handle_history(
    int client_fd,
    MessageHeader *req_header,
    const char *payload,
    int32_t account_id
) {
    cJSON *history = NULL;

    if (history_repo_get(account_id, &history) != 0) {
        printf("[handler] failed to get match history\n");
        return;
    }

    // Convert JSON → string để gửi qua socket / websocket
    char *json_str = cJSON_PrintUnformatted(history);
    if (json_str) {
        printf("[handler] history = %s\n", json_str);

        // TODO: send json_str to client
        free(json_str);
    }

    cJSON_Delete(history);
}