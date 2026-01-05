#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "handlers/room_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "transport/room_manager.h"
#include "db/repo/room_repo.h"
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
    uint8_t wager_enabled;
    uint8_t reserved[4];
} CreateRoomPayload;

typedef struct PACKED {
    uint32_t room_id;
} RoomIDPayload;

typedef struct PACKED {
    uint32_t room_id;
    uint8_t mode;
    uint8_t max_players;
    uint8_t wager_enabled;
    uint8_t reserved;
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
    
    // 4. Validate business rules
    if (data.max_players < 2 || data.max_players > 8) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "max_players must be 2-8");
        return;
    }
    

    
    if (strlen(room_name) == 0) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Room name required");
        return;
    }
    
    // // 4. Escape JSON string
    // const char *safe_name = json_escape_string(room_name);
    
    // // 5. Build JSON for backend
    // char json[1024];
    // snprintf(json, sizeof(json),
    //     "{\"name\":\"%s\",\"visibility\":\"%s\",\"mode\":\"%s\","
    //     "\"max_players\":%u,\"round_time\":%u,\"advanced\":{\"wager\":%s}}",
    //     safe_name,
    //     data.visibility ? "private" : "public",
    //     data.mode ? "elimination" : "scoring",
    //     data.max_players,
    //     data.round_time,
    //     data.wager_enabled ? "true" : "false"
    // );
    
    // // 6. HTTP POST to backend (parsed)
    // char resp_buf[2048];
    // HttpResponse http_resp = http_post_parse("backend", 5000, "/api/room/create",
    //                                          json, resp_buf, sizeof(resp_buf));
    
    // if (http_resp.status_code < 0) {
    //     send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
    //     return;
    // }
    
    // if (http_resp.status_code >= 400) {
    //     send_error(client_fd, req, ERR_BAD_REQUEST, http_resp.body);
    //     return;
    // }
    
    // // 7. Parse room_id from response (simplified: assume {"room_id":123,...})
    // int room_id = 0;
    // if (sscanf(http_resp.body, "{\"success\":true,\"room_id\":%d", &room_id) == 1) {
    //     room_add_member(room_id, client_fd);
    // }
    

    // 4. Call room_repo instead of backend
    char result_payload[2048];
    uint32_t room_id = 0;

    int rc = room_repo_create(
        room_name,
        data.visibility,
        data.mode,
        data.max_players,
        data.wager_enabled,
        result_payload,
        sizeof(result_payload),
        &room_id
    );

    if (rc != 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to create room");
        return;
    }

    // 5. Track host as room member
    room_add_member((int)room_id, client_fd);

    // 6. Prepare response
    uint32_t result_len = strlen(result_payload);

    // forward response
    forward_response(
        client_fd,
        req,
        RES_ROOM_CREATED,
        result_payload,
        result_len
    );

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
    
    // // 3. Build path
    // char path[256];
    // snprintf(path, sizeof(path), "/api/room/%u", room_id);
    
    // // 4. HTTP DELETE (parsed)
    // char resp_buf[1024];
    // HttpResponse http_resp = http_delete_parse("backend", 5000, path,
    //                                            resp_buf, sizeof(resp_buf));
    
    // if (http_resp.status_code < 0) {
    //     send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
    //     return;
    // }
    
    // if (http_resp.status_code >= 400) {
    //     send_error(client_fd, req, ERR_BAD_REQUEST, http_resp.body);
    //     return;
    // }
    char resp_buf[1024];

    int rc = room_repo_close(
        room_id,
        resp_buf,
        sizeof(resp_buf)
    );

    if (rc != 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to close room");
        return;
    }

    // 5. Broadcast success to all members
    room_broadcast(room_id, NTF_ROOM_CLOSED,
              resp_buf, strlen(resp_buf), -1);

    // 6. Forward response to host
    forward_response(client_fd, req, RES_ROOM_CLOSED,
                    resp_buf, strlen(resp_buf));

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
    
   
    
    char resp_buf[2048];

    int rc = room_repo_set_rules(
        room_id,
        data.mode,
        data.max_players,
        data.wager_enabled,
        resp_buf,
        sizeof(resp_buf)
    );

    if (rc != 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to update rules");
        return;
    }

    // 6. Broadcast success to all members (except host)
    room_broadcast(room_id, NTF_RULES_CHANGED,
              resp_buf, strlen(resp_buf), client_fd);

    // 7. Forward response to host
    forward_response(client_fd, req, RES_RULES_UPDATED,
                    resp_buf, strlen(resp_buf));

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
    
   
    
    // // 3. Build JSON
    // snprintf(json, ...);

    // // 4. HTTP POST (parsed)
    // HttpResponse http_resp = http_post_parse(...);

    // if (http_resp.status_code < 0) ...
    // if (http_resp.status_code >= 400) ...



    char resp_buf[1024];

    int rc = room_repo_kick_member(
        room_id,
        target_id,
        resp_buf,
        sizeof(resp_buf)
    );

    if (rc != 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to kick member");
        return;
    }

    // 5. Broadcast success to all members
    room_broadcast(room_id, NTF_MEMBER_KICKED,
              resp_buf, strlen(resp_buf), -1);

    // 6. Forward response to host
    forward_response(client_fd, req, RES_MEMBER_KICKED,
                    resp_buf, strlen(resp_buf));

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
    
    // // 3. Build path
    // char path[256];
    // snprintf(path, sizeof(path), "/api/room/%u/leave", room_id);
    
    // // 4. HTTP POST (parsed)
    // char resp_buf[1024];
    // HttpResponse http_resp = http_post_parse("backend", 5000, path, "{}",
    //                                         resp_buf, sizeof(resp_buf));
    
    // if (http_resp.status_code < 0) {
    //     send_error(client_fd, req, ERR_SERVER_ERROR, "Backend unreachable");
    //     return;
    // }
    
    // if (http_resp.status_code >= 400) {
    //     send_error(client_fd, req, ERR_BAD_REQUEST, http_resp.body);
    //     return;
    // }

    char resp_buf[1024];

    int rc = room_repo_leave(
        room_id,
        resp_buf,
        sizeof(resp_buf)
    );

    if (rc != 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to leave room");
        return;
    }

    
    // 5. Broadcast NTF_PLAYER_LEFT to remaining members
    // room_broadcast(room_id, NTF_PLAYER_LEFT, http_resp.body,
    //               http_resp.body_length, client_fd);
    room_broadcast(room_id, NTF_PLAYER_LEFT,
              resp_buf, strlen(resp_buf), client_fd);
    // 6. Remove from room tracking
    room_remove_member(room_id, client_fd);
    
    // // 7. Forward response
    // forward_response(client_fd, req, RES_ROOM_LEFT,
    //                 http_resp.body, http_resp.body_length);
    

    forward_response(client_fd, req, RES_ROOM_LEFT,
                    resp_buf, strlen(resp_buf));

}
