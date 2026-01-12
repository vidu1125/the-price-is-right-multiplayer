#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "handlers/match_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "transport/room_manager.h"
#include "db/repo/match_repo.h"
//==============================================================================
// PAYLOAD DEFINITIONS (Match Handler)
//==============================================================================

#if defined(__GNUC__) || defined(__clang__)
    #define PACKED __attribute__((packed))
#else
    #define PACKED
    #pragma pack(push, 1)
#endif

typedef struct PACKED {
    uint32_t room_id;
} StartGamePayload;

#if !defined(__GNUC__) && !defined(__clang__)
    #pragma pack(pop)
#endif

//==============================================================================
// HELPER: Send error response
//==============================================================================
static void send_error(int client_fd, MessageHeader *req, uint16_t error_code, const char *message) {
    forward_response(client_fd, req, error_code, message, strlen(message));
}

//==============================================================================
// START GAME
//==============================================================================
void handle_start_game(int client_fd, MessageHeader *req, const char *payload) {
    // 1. Validate
    if (req->length != sizeof(StartGamePayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload");
        return;
    }
    
    // 2. Copy & extract
    StartGamePayload data;
    memcpy(&data, payload, sizeof(data));
    uint32_t room_id = ntohl(data.room_id);
    
    // // 3. Build JSON
    // char json[256];
    // snprintf(json, sizeof(json), "{\"room_id\":%u}", room_id);
    
    // // 4. HTTP POST (parsed)
    // char resp_buf[4096];
    // HttpResponse http_resp = http_post_parse("backend", 5000, "/api/match/start",
    //                                         json, resp_buf, sizeof(resp_buf));
    
    // if (http_resp.status_code < 0) {
    //     send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
    //     return;
    // }
    
    // if (http_resp.status_code >= 400) {
    //     send_error(client_fd, req, ERR_BAD_REQUEST, http_resp.body);
    //     return;
    // }
    // 3. Call DB repo (thay backend Python)
    
    char result_payload[4096];

    int rc = match_repo_start(room_id, result_payload, sizeof(result_payload));
    if (rc != 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to start game");
        return;
    }

    
    // // 5. Broadcast NTF_GAME_START to all members (GAME STARTED!)
    // room_broadcast(room_id, NTF_GAME_START, http_resp.body,
    //               http_resp.body_length, -1);
    
    // // TODO: Schedule timer for 3-second countdown
    // // TODO: After countdown, broadcast NTF_ROUND_START with questions
    
    // // 6. Forward response to host
    // forward_response(client_fd, req, RES_GAME_STARTED,
                    // http_resp.body, http_resp.body_length);

    
                    // 4. Broadcast NTF_GAME_START to all members
    uint32_t result_len = strlen(result_payload);                
    room_broadcast(
        room_id,
        NTF_GAME_START,
        result_payload,
        result_len,
        -1
    );

    // 5. Forward response to host
    forward_response(
        client_fd,
        req,
        RES_GAME_STARTED,
        result_payload,
        result_len
    );
}
