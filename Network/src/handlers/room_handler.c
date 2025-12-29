#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "handlers/room_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "utils/http_utils.h"
#include "utils/json_utils.h"
#include "transport/room_manager.h"

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
    
    // 4. Escape JSON string
    const char *safe_name = json_escape_string(room_name);
    
    // 5. Build JSON for backend
    char json[1024];
    snprintf(json, sizeof(json),
        "{\"name\":\"%s\",\"visibility\":\"%s\",\"mode\":\"%s\","
        "\"max_players\":%u,\"round_time\":%u,\"advanced\":{\"wager\":%s}}",
        safe_name,
        data.visibility ? "private" : "public",
        data.mode ? "elimination" : "scoring",
        data.max_players,
        data.round_time,
        data.wager_enabled ? "true" : "false"
    );
    
    // 6. HTTP POST to backend (parsed)
    char resp_buf[2048];
    HttpResponse http_resp = http_post_parse("backend", 5000, "/api/room/create",
                                             json, resp_buf, sizeof(resp_buf));
    
    if (http_resp.status_code < 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    if (http_resp.status_code >= 400) {
        send_error(client_fd, req, ERR_BAD_REQUEST, http_resp.body);
        return;
    }
    
    // 7. Parse room_id from response (simplified: assume {"room_id":123,...})
    int room_id = 0;
    if (sscanf(http_resp.body, "{\"success\":true,\"room_id\":%d", &room_id) == 1) {
        room_add_member(room_id, client_fd);
    }
    
    // 8. Forward response (body only)
    forward_response(client_fd, req, RES_ROOM_CREATED, 
                    http_resp.body, http_resp.body_length);
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
    
    // 4. HTTP DELETE (parsed)
    char resp_buf[1024];
    HttpResponse http_resp = http_delete_parse("backend", 5000, path,
                                               resp_buf, sizeof(resp_buf));
    
    if (http_resp.status_code < 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    if (http_resp.status_code >= 400) {
        send_error(client_fd, req, ERR_BAD_REQUEST, http_resp.body);
        return;
    }
    
    // 5. Broadcast NTF_ROOM_CLOSED to all members
    room_broadcast(room_id, NTF_ROOM_CLOSED, http_resp.body, 
                  http_resp.body_length, -1);
    
    // 6. Forward response to host
    forward_response(client_fd, req, RES_ROOM_CLOSED, 
                    http_resp.body, http_resp.body_length);
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
    
    // 4. Build path
    char path[256];
    snprintf(path, sizeof(path), "/api/room/%u/rules", room_id);
    
    // 5. HTTP PUT (parsed)
    char resp_buf[2048];
    HttpResponse http_resp = http_put_parse("backend", 5000, path, json,
                                           resp_buf, sizeof(resp_buf));
    
    if (http_resp.status_code < 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    if (http_resp.status_code >= 400) {
        send_error(client_fd, req, ERR_BAD_REQUEST, http_resp.body);
        return;
    }
    
    // 6. Broadcast NTF_RULES_CHANGED to all members (except host)
    room_broadcast(room_id, NTF_RULES_CHANGED, http_resp.body,
                  http_resp.body_length, client_fd);
    
    // 7. Forward response to host
    forward_response(client_fd, req, RES_RULES_UPDATED,
                    http_resp.body, http_resp.body_length);
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
    
    // 4. HTTP POST (parsed)
    char resp_buf[1024];
    HttpResponse http_resp = http_post_parse("backend", 5000, "/api/room/kick",
                                            json, resp_buf, sizeof(resp_buf));
    
    if (http_resp.status_code < 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    if (http_resp.status_code >= 400) {
        send_error(client_fd, req, ERR_BAD_REQUEST, http_resp.body);
        return;
    }
    
    // 5. Broadcast NTF_MEMBER_KICKED to all members
    room_broadcast(room_id, NTF_MEMBER_KICKED, http_resp.body,
                  http_resp.body_length, -1);
    
    // 6. Remove kicked member from room tracking
    // Note: We don't have target_fd here, would need session tracking
    // For now, rely on kicked client to handle NTF and disconnect
    
    // 7. Forward response to host
    forward_response(client_fd, req, RES_MEMBER_KICKED,
                    http_resp.body, http_resp.body_length);
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
    
    // 4. HTTP POST (parsed)
    char resp_buf[1024];
    HttpResponse http_resp = http_post_parse("backend", 5000, path, "{}",
                                            resp_buf, sizeof(resp_buf));
    
    if (http_resp.status_code < 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
        return;
    }
    
    if (http_resp.status_code >= 400) {
        send_error(client_fd, req, ERR_BAD_REQUEST, http_resp.body);
        return;
    }
    
    // 5. Broadcast NTF_PLAYER_LEFT to remaining members
    room_broadcast(room_id, NTF_PLAYER_LEFT, http_resp.body,
                  http_resp.body_length, client_fd);
    
    // 6. Remove from room tracking
    room_remove_member(room_id, client_fd);
    
    // 7. Forward response
    forward_response(client_fd, req, RES_ROOM_LEFT,
                    http_resp.body, http_resp.body_length);
}
