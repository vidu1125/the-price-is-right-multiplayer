#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "handlers/room_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "utils/http_utils.h"

//==============================================================================
// PAYLOAD DEFINITIONS (Room Handler)
//==============================================================================

#if defined(__GNUC__) || defined(__clang__)
    #define PACKED __attribute__((packed))
#else
    #define PACKED
    #pragma pack(push, 1)
#endif

typedef struct PACKED {
    char name[64];
    uint8_t visibility;
    uint8_t mode;
    uint8_t max_players;
    uint8_t round_time;
    uint8_t wager_enabled;
    uint8_t reserved[3];
} CreateRoomPayload;

typedef struct PACKED {
    uint32_t room_id;
} RoomIDPayload;

typedef struct PACKED {
    uint32_t room_id;
    uint8_t mode;
    uint8_t max_players;
    uint8_t round_time;
    uint8_t wager_enabled;
} SetRulesPayload;

typedef struct PACKED {
    uint32_t room_id;
    uint32_t target_id;
} KickMemberPayload;

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
// CREATE ROOM
//==============================================================================
void handle_create_room(int client_fd, MessageHeader *req, const char *payload) {
    // 1. Validate payload size
    if (req->length != sizeof(CreateRoomPayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload size");
        return;
    }
    
    // 2. Copy to local struct (NOT cast!)
    CreateRoomPayload data;
    memcpy(&data, payload, sizeof(data));
    
    // 3. Null-terminate string (safety)
    char room_name[65];
    memcpy(room_name, data.name, 64);
    room_name[64] = '\0';
    
    // 4. Build JSON for backend
    char json[1024];
    snprintf(json, sizeof(json),
        "{\"name\":\"%s\",\"visibility\":\"%s\",\"mode\":\"%s\","
        "\"max_players\":%u,\"round_time\":%u,\"advanced\":{\"wager\":%s}}",
        room_name,
        data.visibility ? "private" : "public",
        data.mode ? "elimination" : "scoring",
        data.max_players,
        data.round_time,
        data.wager_enabled ? "true" : "false"
    );
    
    // 5. HTTP POST to backend
    char resp_buf[2048];
    int resp_len = http_post("backend", 5000, "/api/room/create", 
                             json, resp_buf, sizeof(resp_buf));
    
    if (resp_len <= 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    // 6. Forward response
    forward_response(client_fd, req, RES_ROOM_CREATED, resp_buf, resp_len);
}

//==============================================================================
// CLOSE ROOM (Host only)
//==============================================================================
void handle_close_room(int client_fd, MessageHeader *req, const char *payload) {
    // 1. Validate
    if (req->length != sizeof(RoomIDPayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload");
        return;
    }
    
    // 2. Copy & extract
    RoomIDPayload data;
    memcpy(&data, payload, sizeof(data));
    uint32_t room_id = ntohl(data.room_id);
    
    // 3. Build path
    char path[256];
    snprintf(path, sizeof(path), "/api/room/%u", room_id);
    
    // 4. HTTP DELETE
    char resp_buf[1024];
    int resp_len = http_delete("backend", 5000, path, resp_buf, sizeof(resp_buf));
    
    if (resp_len <= 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    // TODO: Parse response & broadcast NTF_ROOM_CLOSED if success
    
    // 5. Forward response
    forward_response(client_fd, req, RES_ROOM_CLOSED, resp_buf, resp_len);
}

//==============================================================================
// SET RULES (Host only)
//==============================================================================
void handle_set_rules(int client_fd, MessageHeader *req, const char *payload) {
    // 1. Validate
    if (req->length != sizeof(SetRulesPayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload");
        return;
    }
    
    // 2. Copy & extract
    SetRulesPayload data;
    memcpy(&data, payload, sizeof(data));
    uint32_t room_id = ntohl(data.room_id);
    
    // 3. Build JSON
    char json[512];
    snprintf(json, sizeof(json),
        "{\"room_id\":%u,\"rules\":{\"mode\":\"%s\",\"max_players\":%u,"
        "\"round_time\":%u,\"advanced\":{\"wager\":%s}}}",
        room_id,
        data.mode ? "elimination" : "scoring",
        data.max_players,
        data.round_time,
        data.wager_enabled ? "true" : "false"
    );
    
    // 4. HTTP PUT
    char resp_buf[1024];
    int resp_len = http_put("backend", 5000, "/api/room/rules",
                           json, resp_buf, sizeof(resp_buf));
    
    if (resp_len <= 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    // TODO: Parse response & broadcast NTF_RULES_CHANGED if success
    
    // 5. Forward response
    forward_response(client_fd, req, RES_RULES_UPDATED, resp_buf, resp_len);
}

//==============================================================================
// KICK MEMBER (Host only)
//==============================================================================
void handle_kick_member(int client_fd, MessageHeader *req, const char *payload) {
    // 1. Validate
    if (req->length != sizeof(KickMemberPayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload");
        return;
    }
    
    // 2. Copy & extract
    KickMemberPayload data;
    memcpy(&data, payload, sizeof(data));
    uint32_t room_id = ntohl(data.room_id);
    uint32_t target_id = ntohl(data.target_id);
    
    // 3. Build JSON
    char json[256];
    snprintf(json, sizeof(json),
        "{\"room_id\":%u,\"target_id\":%u}", room_id, target_id);
    
    // 4. HTTP POST
    char resp_buf[1024];
    int resp_len = http_post("backend", 5000, "/api/room/kick",
                            json, resp_buf, sizeof(resp_buf));
    
    if (resp_len <= 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    // TODO: Parse response & send NTF_MEMBER_KICKED + NTF_PLAYER_LEFT
    
    // 5. Forward response
    forward_response(client_fd, req, RES_MEMBER_KICKED, resp_buf, resp_len);
}

//==============================================================================
// LEAVE ROOM (Any member)
//==============================================================================
void handle_leave_room(int client_fd, MessageHeader *req, const char *payload) {
    // 1. Validate
    if (req->length != sizeof(RoomIDPayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload");
        return;
    }
    
    // 2. Copy & extract
    RoomIDPayload data;
    memcpy(&data, payload, sizeof(data));
    uint32_t room_id = ntohl(data.room_id);
    
    // 3. Build path
    char path[256];
    snprintf(path, sizeof(path), "/api/room/%u/leave", room_id);
    
    // 4. HTTP POST
    char resp_buf[1024];
    int resp_len = http_post("backend", 5000, path, "{}", 
                             resp_buf, sizeof(resp_buf));
    
    if (resp_len <= 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    // TODO: Parse response & broadcast NTF_PLAYER_LEFT
    
    // 5. Forward response
    forward_response(client_fd, req, RES_ROOM_LEFT, resp_buf, resp_len);
}
