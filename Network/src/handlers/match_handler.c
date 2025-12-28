#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "handlers/match_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "utils/http_utils.h"

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
    
    // 3. Build JSON
    char json[256];
    snprintf(json, sizeof(json), "{\"room_id\":%u}", room_id);
    
    // 4. HTTP POST
    char resp_buf[4096];
    int resp_len = http_post("backend", 5000, "/api/match/start",
                            json, resp_buf, sizeof(resp_buf));
    
    if (resp_len <= 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    // TODO: Parse response & broadcast NTF_GAME_START
    // TODO: Schedule NTF_ROUND_START after countdown
    
    // 5. Forward response
    forward_response(client_fd, req, RES_GAME_STARTED, resp_buf, resp_len);
}
