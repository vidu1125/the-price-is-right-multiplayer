#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // free

#include "handlers/history_handler.h"
#include "protocol/opcode.h"
#include "db/repo/question_repo.h"
#include "protocol/protocol.h"




void handle_history(
    int client_fd,
    MessageHeader *req_header,
    const char *payload,
    int32_t account_id
) {
   
    if (req_header->length < sizeof(HistRequestPayload)) {
        printf("[HANDLER] invalid history request size: %u\n", req_header->length);
        return;
    }

    const HistRequestPayload *req = (const HistRequestPayload *)payload;
    printf("[HANDLER] view history limit=%u offset=%u\n", req->limit, req->reserved);

    // Create mock history records
    // In production, we would query DB and count items
    int record_count = 2;
    size_t payload_len = sizeof(HistResponsePayload) + (record_count * sizeof(HistoryRecord));
    
    HistResponsePayload *resp_payload = (HistResponsePayload *)malloc(payload_len);
    if (!resp_payload) {
        printf("[handler] failed to alloc history response\n");
        return;
    }

    resp_payload->count = record_count;
    resp_payload->reserved = 0;

    // Item 1
    resp_payload->records[0].match_id = 1023;
    resp_payload->records[0].final_score = 120;
    resp_payload->records[0].mode = 1; // Scoring (example enum)
    resp_payload->records[0].is_winner = 1;
    strncpy(resp_payload->records[0].ranking, "1st", 8);
    resp_payload->records[0].ended_at = 1700000000;

    // Item 2
    resp_payload->records[1].match_id = 1022;
    resp_payload->records[1].final_score = -40;
    resp_payload->records[1].mode = 2; // Eliminated (example enum)
    resp_payload->records[1].is_winner = 0;
    strncpy(resp_payload->records[1].ranking, "4th", 8);
    resp_payload->records[1].ended_at = 1700000000;

    printf("[handler] sending history binary: %d records, %zu bytes\n", record_count, payload_len);

    forward_response(
        client_fd,
        req_header,
        CMD_HIST,
        (const char *)resp_payload,
        payload_len
    );

    free(resp_payload);
}