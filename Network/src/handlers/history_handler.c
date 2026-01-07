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
   
    cJSON *root = cJSON_CreateArray();
    if (!root) {
        printf("[handler] failed to create cJSON array\n");
        return;
    }

    // Mock item 1
    cJSON *item1 = cJSON_CreateObject();
    cJSON_AddNumberToObject(item1, "matchID", 1023);
    cJSON_AddNumberToObject(item1, "score", 120);
    cJSON_AddStringToObject(item1, "mode", "Scoring");
    cJSON_AddStringToObject(item1, "ranking", "1st");
    cJSON_AddBoolToObject(item1, "isWinner", 1);
    cJSON_AddItemToArray(root, item1);

    // Mock item 2
    cJSON *item2 = cJSON_CreateObject();
    cJSON_AddNumberToObject(item2, "matchID", 1022);
    cJSON_AddNumberToObject(item2, "score", -40);
    cJSON_AddStringToObject(item2, "mode", "Eliminated");
    cJSON_AddStringToObject(item2, "ranking", "4th");
    cJSON_AddBoolToObject(item2, "isWinner", 0);
    cJSON_AddItemToArray(root, item2);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        printf("[handler] history response: %s\n", json_str);

        uint32_t payload_len = strlen(json_str);

        forward_response(
            client_fd,
            req_header,
            CMD_HIST,
            json_str,
            payload_len
        );

        free(json_str);
    }

    cJSON_Delete(root);
}