#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "handlers/room_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "transport/room_manager.h"
#include "db/repo/room_repo.h"
#include "handlers/session_manager.h"

//==============================================================================
// PAYLOAD DEFINITIONS (Room Handler)
//==============================================================================

#if defined(__GNUC__) || defined(__clang__)
    #define PACKED __attribute__((packed))
#else
    #define PACKED
    #pragma pack(push, 1)
#endif

// CREATE ROOM (36 bytes)
typedef struct PACKED {
    char name[32];
    uint8_t mode;
    uint8_t max_players;
    uint8_t visibility;
    uint8_t wager_mode;
} CreateRoomPayload;

// CREATE ROOM RESPONSE (12 bytes)
typedef struct PACKED {
    uint32_t room_id;      // network byte order
    char room_code[8];
} CreateRoomResponsePayload;

// Other payloads (keep for now)
typedef struct PACKED {
    uint32_t room_id;
} RoomIDPayload;

typedef struct PACKED {
    uint8_t mode;
    uint8_t max_players;
    uint8_t visibility;
    uint8_t wager_mode;
} SetRulesPayload;

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
    printf("[SERVER] [CREATE_ROOM] Request from fd=%d\n", client_fd);
    
    // STEP 1: Validate session & state
    UserSession *session = session_get_by_socket(client_fd);
    
    if (!session || session->state == SESSION_UNAUTHENTICATED) {
        printf("[SERVER] [CREATE_ROOM] Error: Not logged in (fd=%d)\n", client_fd);
        send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }
    
    if (session->state != SESSION_LOBBY) {
        printf("[SERVER] [CREATE_ROOM] Error: Invalid state %d (fd=%d)\n", session->state, client_fd);
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid state");
        return;
    }
    
    // STEP 2: Check user not in any room
    if (room_user_in_any_room(session->account_id)) {
        printf("[SERVER] [CREATE_ROOM] Error: User %u already in a room\n", session->account_id);
        send_error(client_fd, req, ERR_BAD_REQUEST, "Already in a room");
        return;
    }
    
    // STEP 3: Validate payload size
    if (req->length != sizeof(CreateRoomPayload)) {
        printf("[SERVER] [CREATE_ROOM] Error: Invalid payload size %u, expected %lu\n", 
               req->length, sizeof(CreateRoomPayload));
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload size");
        return;
    }
    
    // STEP 4: Parse payload
    CreateRoomPayload data;
    memcpy(&data, payload, sizeof(data));
    
    char room_name[33];
    memcpy(room_name, data.name, 32);
    room_name[32] = '\0';
    
    printf("[SERVER] [CREATE_ROOM] Parsed: name='%s', mode=%u, max_players=%u, visibility=%u, wager=%u\n",
           room_name, data.mode, data.max_players, data.visibility, data.wager_mode);
    
    // STEP 5: Validate business rules
    if (strlen(room_name) == 0 || strlen(room_name) > 32) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid room name");
        return;
    }
    
    if (data.mode == MODE_ELIMINATION) {
        if (data.max_players != 4) {
            send_error(client_fd, req, ERR_BAD_REQUEST, 
                       "ELIMINATION requires exactly 4 players");
            return;
        }
    } else if (data.mode == MODE_SCORING) {
        if (data.max_players < 4 || data.max_players > 6) {
            send_error(client_fd, req, ERR_BAD_REQUEST, 
                       "SCORING requires 4-6 players");
            return;
        }
    } else {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid mode");
        return;
    }
    
    if (data.visibility != ROOM_PUBLIC && data.visibility != ROOM_PRIVATE) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid visibility");
        return;
    }
    
    if (data.wager_mode != 0 && data.wager_mode != 1) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid wager mode");
        return;
    }
    
    // STEP 6: Create RoomState (in-memory)
    RoomState *room = room_create();
    if (!room) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Cannot create room");
        return;
    }
    
    strncpy(room->name, room_name, 32);
    room->host_id = session->account_id;
    room->status = ROOM_WAITING;
    room->mode = (GameMode)data.mode;
    room->max_players = data.max_players;
    room->visibility = (RoomVisibility)data.visibility;
    room->wager_mode = data.wager_mode;
    
    // STEP 7: Call DB to get ID and code
    char db_room_code[9] = {0};
    uint32_t db_room_id = 0;
    
    int rc = room_repo_create(
        session->account_id,
        room_name,
        data.visibility,
        data.mode,
        data.max_players,
        data.wager_mode,
        db_room_code,
        sizeof(db_room_code),
        &db_room_id
    );
    
    if (rc != 0) {
        room_destroy(room->id);
        send_error(client_fd, req, ERR_SERVER_ERROR, "DB error");
        return;
    }
    
    // Update room with DB values
    room->id = db_room_id;
    strncpy(room->code, db_room_code, 8);
    
    printf("[SERVER] [CREATE_ROOM] DB persisted: id=%u, code=%s\n", room->id, room->code);
    
    // STEP 8: Add host as player
    room_add_player(room->id, session->account_id, client_fd);
    room->players[0].is_host = true;
    
    printf("[SERVER] [CREATE_ROOM] Host added as player (account_id=%u)\n", session->account_id);
    
    // STEP 9: Send response
    CreateRoomResponsePayload resp;
    resp.room_id = htonl(room->id);
    memcpy(resp.room_code, room->code, 8);
    
    forward_response(client_fd, req, RES_ROOM_CREATED, 
                     (char*)&resp, sizeof(resp));
    
    printf("[SERVER] [CREATE_ROOM] ✅ SUCCESS: room_id=%u, code=%s, name='%s'\n", 
           room->id, room->code, room->name);
    
    // STEP 10: Broadcast NTF_PLAYER_LIST
    // TODO: Implement in next phase
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
// SET RULES (Host only) - TODO: Implement later
//==============================================================================
void handle_set_rules(int client_fd, MessageHeader *req, const char *payload) {
    (void)payload; // Unused
    send_error(client_fd, req, ERR_BAD_REQUEST, "Not implemented yet");
    /*
    // 1. Validate
    if (req->length != sizeof(SetRulesPayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload");
        return;
    }
    
    // 2. Copy & extract
    SetRulesPayload data;
    memcpy(&data, payload, sizeof(data));
    // TODO: Get room_id from session instead
    
    char resp_buf[2048];

    int rc = room_repo_set_rules(
        room_id,
        data.mode,
        data.max_players,
        data.wager_mode,
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
    */
}

//==============================================================================
// KICK MEMBER (Host only)
//==============================================================================
void handle_kick_member(int client_fd, MessageHeader *req, const char *payload) {
    (void)payload; // Unused
    send_error(client_fd, req, ERR_BAD_REQUEST, "Not implemented yet");
    /*
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
    */
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
