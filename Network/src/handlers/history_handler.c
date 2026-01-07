#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // free

#include "handlers/history_handler.h"
#include "protocol/opcode.h"
#include "db/repo/question_repo.h"
#include "db/repo/match_repo.h"
#include "protocol/protocol.h"




void handle_history(
    int client_fd,
    MessageHeader *req_header,
    const char *payload,
    int32_t account_id
) {
   
    if (req_header->length < sizeof(HistRequestPayload)) {
        printf("[HANDLER] <viewHistory> invalid history request size: %u\n", req_header->length);
        return;
    }

    const HistRequestPayload *req = (const HistRequestPayload *)payload;
    printf("[HANDLER] <viewHistory> limit=%u offset=%u\n", req->limit, req->reserved);

    int count = 0;
    HistoryRecord *db_records = NULL;
    
    db_error_t err = db_match_get_history(account_id, req->limit, req->reserved, &db_records, &count);
    

    if (err != DB_SUCCESS) {
        printf("[HANDLER] <viewHistory> DB Error: %d\n", err);
        count = 0; 
    }

    printf("[HANDLER] <viewHistory> Found %d records\n", count);

    size_t payload_len = sizeof(HistResponsePayload) + (count * sizeof(HistoryRecord));
    HistResponsePayload *resp_payload = (HistResponsePayload *)malloc(payload_len);
    
    if (!resp_payload) {
        printf("[HANDLER] <viewHistory> Failed to alloc response\n");
        if (db_records) free(db_records);
        return;
    }

    resp_payload->count = (uint8_t)count;
    resp_payload->reserved = 0;

    if (count > 0 && db_records) {
        memcpy(resp_payload->records, db_records, count * sizeof(HistoryRecord));
        free(db_records); // Free the repo buffer
    }

    printf("[HANDLER] <viewHistory> Sending binary: %d records, %zu bytes\n", count, payload_len);

    forward_response(
        client_fd,
        req_header,
        CMD_HIST,
        (const char *)resp_payload,
        payload_len
    );

    free(resp_payload);
}