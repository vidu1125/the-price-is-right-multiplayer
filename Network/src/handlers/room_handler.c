#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "handlers/room_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "transport/room_manager.h"
#include "db/repo/room_repo.h"
#include "handlers/session_manager.h"
#include "db/core/db_client.h"
#include <cjson/cJSON.h>
#include <limits.h>
#include <stdlib.h>

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
    uint32_t target_account_id; // network byte order
} KickMemberPayload;

typedef struct PACKED {
    uint8_t mode;
    uint8_t max_players;
    uint8_t visibility;
    uint8_t wager_mode;
} SetRulesPayload;

// JOIN ROOM (16 bytes fixed)
typedef struct PACKED {
    uint8_t  by_code;      // 0 = join by room_id, 1 = join by room_code
    uint8_t  reserved[3];  // padding for alignment
    uint32_t room_id;      // network byte order - used if by_code = 0
    char     room_code[8]; // null-terminated - used if by_code = 1
} JoinRoomPayload;

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
    
    // STEP 8: Fetch host profile to get name & avatar
    char profile_name[64] = "Host";
    char profile_avatar[256] = "";
    char query[128];
    snprintf(query, sizeof(query), "SELECT * FROM profiles WHERE account_id = %u LIMIT 1", session->account_id);
    
    cJSON *profile_response = NULL;
    db_error_t profile_rc = db_get("profiles", query, &profile_response);
    
    if (profile_rc == DB_OK && profile_response) {
        cJSON *first = cJSON_GetArrayItem(profile_response, 0);
        if (first) {
            cJSON *name_item = cJSON_GetObjectItem(first, "name");
            if (name_item && cJSON_IsString(name_item)) {
                strncpy(profile_name, name_item->valuestring, sizeof(profile_name) - 1);
                profile_name[sizeof(profile_name) - 1] = '\0';
            }
            cJSON *avatar_item = cJSON_GetObjectItem(first, "avatar");
            if (avatar_item && cJSON_IsString(avatar_item)) {
                strncpy(profile_avatar, avatar_item->valuestring, sizeof(profile_avatar) - 1);
                profile_avatar[sizeof(profile_avatar) - 1] = '\0';
            }
        }
        cJSON_Delete(profile_response);
    }
    
    // STEP 9: Add host as player with name and avatar
    room_add_player(room->id, session->account_id, profile_name, profile_avatar, client_fd);
    room->players[0].is_host = true;
    
    printf("[SERVER] [CREATE_ROOM] Host added as player (account_id=%u, name=%s)\n", 
           session->account_id, profile_name);
    
    // STEP 9: Send response
    CreateRoomResponsePayload resp;
    resp.room_id = htonl(room->id);
    memcpy(resp.room_code, room->code, 8);
    
    forward_response(client_fd, req, RES_ROOM_CREATED, 
                     (char*)&resp, sizeof(resp));
    
    printf("[SERVER] [CREATE_ROOM] ✅ SUCCESS: room_id=%u, code=%s, name='%s'\n", 
           room->id, room->code, room->name);
    
    // STEP 10: Broadcast NTF_PLAYER_LIST
    broadcast_player_list(room->id);
}

//==============================================================================
// JOIN ROOM
//==============================================================================
void handle_join_room(int client_fd, MessageHeader *req, const char *payload) {
    printf("[SERVER] [JOIN_ROOM] Request from fd=%d\n", client_fd);
    
    // STEP 1: Validate session & state
    UserSession *session = session_get_by_socket(client_fd);
    
    if (!session || session->state == SESSION_UNAUTHENTICATED) {
        printf("[SERVER] [JOIN_ROOM] Error: Not logged in (fd=%d)\n", client_fd);
        send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }
    
    if (session->state != SESSION_LOBBY) {
        printf("[SERVER] [JOIN_ROOM] Error: Invalid state %d (fd=%d)\n", session->state, client_fd);
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid state");
        return;
    }
    
    // STEP 2: Check user not in any room
    if (room_user_in_any_room(session->account_id)) {
        printf("[SERVER] [JOIN_ROOM] Error: User %u already in a room\n", session->account_id);
        send_error(client_fd, req, ERR_BAD_REQUEST, "Already in a room");
        return;
    }
    
    // STEP 3: Validate payload size
    if (req->length != sizeof(JoinRoomPayload)) {
        printf("[SERVER] [JOIN_ROOM] Error: Invalid payload size %u, expected %lu\n", 
               req->length, sizeof(JoinRoomPayload));
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload size");
        return;
    }
    
    // STEP 4: Parse payload
    JoinRoomPayload data;
    memcpy(&data, payload, sizeof(data));
    
    uint32_t target_room_id = ntohl(data.room_id);
    
    printf("[SERVER] [JOIN_ROOM] by_code=%d, room_id=%u\n", data.by_code, target_room_id);
    
    // STEP 5: Find room
    RoomState *room = NULL;
    
    if (data.by_code == 0) {
        // Join by room_id (from public list)
        room = room_get(target_room_id);
        
        if (!room) {
            printf("[SERVER] [JOIN_ROOM] Error: Room %u not found\n", target_room_id);
            send_error(client_fd, req, ERR_BAD_REQUEST, "Room not found");
            return;
        }
        
        // IMPORTANT: Join from list requires PUBLIC room
        if (room->visibility != ROOM_PUBLIC) {
            printf("[SERVER] [JOIN_ROOM] Error: Room %u is private\n", target_room_id);
            send_error(client_fd, req, ERR_BAD_REQUEST, "Room is private");
            return;
        }
        
    } else {
        // Join by room_code (enter private code)
        char room_code[9];
        memcpy(room_code, data.room_code, 8);
        room_code[8] = '\0';
        
        printf("[SERVER] [JOIN_ROOM] Looking for room with code '%s'\n", room_code);
        
        room = find_room_by_code(room_code);
        
        if (!room) {
            printf("[SERVER] [JOIN_ROOM] Error: Invalid room code '%s'\n", room_code);
            send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid room code");
            return;
        }
        
        // TODO: Implement invite system for private rooms
        // For now, block all private room joins
        if (room->visibility != ROOM_PUBLIC) {
            printf("[SERVER] [JOIN_ROOM] Error: Private room requires invite (not implemented)\n");
            send_error(client_fd, req, ERR_BAD_REQUEST, "Private room requires invite");
            return;
        }
    }
    
    printf("[SERVER] [JOIN_ROOM] Found room %u (%s), status=%d, players=%d/%d\n",
           room->id, room->code, room->status, room->player_count, room->max_players);
    
    // STEP 6: Validate room status & capacity
    if (room->status != ROOM_WAITING) {
        printf("[SERVER] [JOIN_ROOM] Error: Game already started (status=%d)\n", room->status);
        send_error(client_fd, req, ERR_GAME_STARTED, "Game already started");
        return;
    }
    
    // Check capacity based on mode
    if (room->mode == MODE_ELIMINATION) {
        if (room->player_count >= 4) {
            printf("[SERVER] [JOIN_ROOM] Error: Room full (elimination mode, 4/4)\n");
            send_error(client_fd, req, ERR_ROOM_FULL, "Room is full");
            return;
        }
    } else { // MODE_SCORING
        if (room->player_count >= room->max_players) {
            printf("[SERVER] [JOIN_ROOM] Error: Room full (%d/%d)\n", 
                   room->player_count, room->max_players);
            send_error(client_fd, req, ERR_ROOM_FULL, "Room is full");
            return;
        }
    }
    
    // STEP 7: Get player name & avatar from DB
    char profile_name[64] = "Player";  // fallback default
    char profile_avatar[256] = "";     // fallback empty
    char query[128];
    snprintf(query, sizeof(query), "SELECT * FROM profiles WHERE account_id = %u LIMIT 1", session->account_id);
    
    cJSON *profile_response = NULL;
    db_error_t profile_rc = db_get("profiles", query, &profile_response);
    
    if (profile_rc == DB_OK && profile_response) {
        cJSON *first = cJSON_GetArrayItem(profile_response, 0);
        if (first) {
            cJSON *name_item = cJSON_GetObjectItem(first, "name");
            if (name_item && cJSON_IsString(name_item)) {
                strncpy(profile_name, name_item->valuestring, sizeof(profile_name) - 1);
                profile_name[sizeof(profile_name) - 1] = '\0';
            }
            cJSON *avatar_item = cJSON_GetObjectItem(first, "avatar");
            if (avatar_item && cJSON_IsString(avatar_item)) {
                strncpy(profile_avatar, avatar_item->valuestring, sizeof(profile_avatar) - 1);
                profile_avatar[sizeof(profile_avatar) - 1] = '\0';
            }
        }
        cJSON_Delete(profile_response);
    }
    
    printf("[SERVER] [JOIN_ROOM] Player name: %s\n", profile_name);
    
    // STEP 8: Add player to room (in-memory)
    int rc = room_add_player(room->id, session->account_id, profile_name, profile_avatar, client_fd);
    
    if (rc != 0) {
        printf("[SERVER] [JOIN_ROOM] Error: Failed to add player to room\n");
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to add player");
        return;
    }
    
    printf("[SERVER] [JOIN_ROOM] Player %u (%s) added to room %u\n",
           session->account_id, profile_name, room->id);
    
    // STEP 9: Save to database (best-effort)
    cJSON *member_payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(member_payload, "room_id", room->id);
    cJSON_AddNumberToObject(member_payload, "account_id", session->account_id);
    
    cJSON *db_response = NULL;
    db_error_t db_rc = db_post("room_members", member_payload, &db_response);
    cJSON_Delete(member_payload);
    
    if (db_rc != DB_OK) {
        printf("[SERVER] [JOIN_ROOM] ⚠️ DB insert failed (non-critical)\n");
        printf("[SERVER] [JOIN_ROOM] Player already in memory, game can proceed\n");
        // Continue anyway - eventual consistency model
    }
    
    if (db_response) {
        cJSON_Delete(db_response);
    }
    
    // STEP 10: Send response to joiner with Room Info (JSON)
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "roomId", room->id);
    cJSON_AddStringToObject(resp, "roomCode", room->code);
    cJSON_AddStringToObject(resp, "roomName", room->name);
    cJSON_AddNumberToObject(resp, "hostId", room->host_id);
    cJSON_AddBoolToObject(resp, "isHost", false); // Joiner is never host
    
    // Add game rules
    cJSON *rules = cJSON_CreateObject();
    cJSON_AddStringToObject(rules, "mode", room->mode == MODE_ELIMINATION ? "elimination" : "scoring");
    cJSON_AddNumberToObject(rules, "maxPlayers", room->max_players);
    cJSON_AddBoolToObject(rules, "wagerMode", room->wager_mode); // Add wager_mode
    cJSON_AddStringToObject(rules, "visibility", room->visibility == ROOM_PUBLIC ? "public" : "private");
    cJSON_AddItemToObject(resp, "gameRules", rules);

    // Add current players (To fix race condition)
    cJSON *players_array = cJSON_CreateArray();
    for (int i = 0; i < room->player_count; i++) {
        RoomPlayerState *p = &room->players[i];
        cJSON *p_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(p_obj, "account_id", p->account_id);
        cJSON_AddStringToObject(p_obj, "name", p->name);
        cJSON_AddStringToObject(p_obj, "avatar", p->avatar[0] ? p->avatar : "");
        cJSON_AddBoolToObject(p_obj, "is_host", p->is_host);
        cJSON_AddBoolToObject(p_obj, "is_ready", p->is_ready);
        cJSON_AddItemToArray(players_array, p_obj);
    }
    cJSON_AddItemToObject(resp, "players", players_array);
    
    char *json_str = cJSON_PrintUnformatted(resp);
    forward_response(client_fd, req, RES_ROOM_JOINED, json_str, strlen(json_str));
    
    free(json_str);
    cJSON_Delete(resp);
    
    printf("[SERVER] [JOIN_ROOM] ✅ SUCCESS: player %u joined room %u\n",
           session->account_id, room->id);
    
    // STEP 11: Broadcast notifications
    // 11.1: NTF_PLAYER_JOINED to existing players (exclude joiner)
    char joined_notif[256];
    int offset = snprintf(joined_notif, sizeof(joined_notif),
        "{\"account_id\":%u,\"name\":\"%s\"}",
        session->account_id, profile_name);
    
    room_broadcast(room->id, NTF_PLAYER_JOINED, joined_notif, offset, client_fd);
    
    // 11.2: NTF_PLAYER_LIST to ALL (including joiner)
    broadcast_player_list(room->id);
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
    printf("[HANDLER] <KICK_MEMBER> Request from fd=%d\n", client_fd);
    
    // ========== BƯỚC 1: VALIDATE PAYLOAD ==========
    if (req->length != sizeof(KickMemberPayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload");
        return;
    }
    
    // ========== BƯỚC 2: PARSE PAYLOAD ==========
    KickMemberPayload data;
    memcpy(&data, payload, sizeof(data));
    uint32_t target_id = ntohl(data.target_account_id);
    
    printf("[Kick Member] Target account_id: %u\n", target_id);
    
    // ========== BƯỚC 3: VALIDATE SESSION ==========
    UserSession *session = session_get_by_socket(client_fd);
    if (!session || session->state != SESSION_LOBBY) {
        send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }
    
    uint32_t kicker_id = session->account_id;
    printf("[Kick Member] Kicker account_id: %u\n", kicker_id);
    
    // ========== BƯỚC 4: TÌM ROOM ==========
    uint32_t room_id = room_find_by_player_account(kicker_id);
    if (room_id == 0) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Not in any room");
        return;
    }
    
    RoomState *room = room_get_state(room_id);
    if (!room) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Room not found");
        return;
    }
    
    printf("[Kick Member] Room ID: %u\n", room_id);
    
    // ========== BƯỚC 5: VALIDATE HOST PERMISSION ==========
    if (room->host_id != kicker_id) {
        send_error(client_fd, req, ERR_NOT_HOST, "Only host can kick members");
        return;
    }
    
    // ========== BƯỚC 6: VALIDATE ROOM STATUS ==========
    if (room->status != ROOM_WAITING) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Cannot kick during game");
        return;
    }
    
    // ========== BƯỚC 7: VALIDATE TARGET TỒN TẠI ==========
    int target_index = -1;
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].account_id == target_id) {
            target_index = i;
            break;
        }
    }
    
    if (target_index == -1) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Target player not in room");
        return;
    }
    
    // ========== BƯỚC 8: KHÔNG THỂ KICK CHÍNH MÌNH ==========
    if (target_id == kicker_id) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Cannot kick yourself");
        return;
    }
    
    // ========== LOG BEFORE STATE ==========
    printf("[Kick Member] ===== BEFORE STATE =====\n");
    printf("[Kick Member] [RoomState] Room ID: %u\n", room->id);
    printf("[Kick Member] [RoomState] Host ID: %u\n", room->host_id);
    printf("[Kick Member] [RoomState] Player Count: %d\n", room->player_count);
    printf("[Kick Member] [RoomPlayerState] Players:\n");
    for (int i = 0; i < room->player_count; i++) {
        printf("[Kick Member] [RoomPlayerState]   [%d] %s (id=%u, host=%s, ready=%s, connected=%s)\n",
               i, room->players[i].name, room->players[i].account_id,
               room->players[i].is_host ? "YES" : "NO",
               room->players[i].is_ready ? "YES" : "NO",
               room->players[i].connected ? "YES" : "NO");
    }
    printf("[Kick Member] Kicker: %u (host)\n", kicker_id);
    printf("[Kick Member] Target: %u\n", target_id);
    
    // ========== BƯỚC 9: TÌM TARGET SESSION VÀ SOCKET ==========
    UserSession *target_session = session_get_by_account(target_id);
    int target_fd = -1;
    if (target_session) {
        target_fd = target_session->socket_fd;
        printf("[Kick Member] Target socket fd: %d\n", target_fd);
    } else {
        printf("[Kick Member] ⚠️  Target session not found (offline?)\n");
    }
    
    // ========== BƯỚC 10: XÓA KHỎI IN-MEMORY (QUAN TRỌNG: PHẢI TRƯỚC KHI BROADCAST!) ==========
    int removed = room_remove_player(room_id, target_id);
    if (removed != 0) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Failed to remove player");
        return;
    }
    
    printf("[Kick Member] ✅ Removed player %u from in-memory state\n", target_id);
    
    // ========== BƯỚC 11: XÓA KHỎI DATABASE ==========
    int rc = room_repo_remove_player(room_id, target_id);
    if (rc != 0) {
        printf("[Kick Member] ⚠️  Warning: Failed to remove from DB\n");
    }
    
    // ========== BƯỚC 12: CẬP NHẬT TARGET SESSION STATE ==========
    if (target_session) {
        target_session->state = SESSION_LOBBY;
        printf("[Kick Member] Updated target session state to SESSION_LOBBY\n");
    }
    
    // ========== LOG AFTER STATE ==========
    printf("[Kick Member] ===== AFTER STATE =====\n");
    printf("[Kick Member] [RoomState] Room ID: %u\n", room->id);
    printf("[Kick Member] [RoomState] Player Count: %d\n", room->player_count);
    printf("[Kick Member] [RoomPlayerState] Players:\n");
    for (int i = 0; i < room->player_count; i++) {
        printf("[Kick Member] [RoomPlayerState]   [%d] %s (id=%u, host=%s, ready=%s, connected=%s)\n",
               i, room->players[i].name, room->players[i].account_id,
               room->players[i].is_host ? "YES" : "NO",
               room->players[i].is_ready ? "YES" : "NO",
               room->players[i].connected ? "YES" : "NO");
    }
    
    // ========== BƯỚC 13: GỬI RESPONSE CHO HOST ==========
    forward_response(client_fd, req, RES_MEMBER_KICKED, "", 0);
    printf("[Kick Member] Sent RES_MEMBER_KICKED to host (fd=%d)\n", client_fd);
    
    // ========== BƯỚC 14: GỬI NTF_MEMBER_KICKED CHO NGƯỜI BỊ KICK (UNICAST) ==========
    if (target_fd >= 0) {
        cJSON *kick_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(kick_json, "room_id", room_id);
        char *kick_str = cJSON_PrintUnformatted(kick_json);
        
        // Manual send (vì không có helper function cho unicast)
        MessageHeader ntf;
        memset(&ntf, 0, sizeof(ntf));
        ntf.magic = htons(MAGIC_NUMBER);
        ntf.version = PROTOCOL_VERSION;
        ntf.command = htons(NTF_MEMBER_KICKED);
        ntf.seq_num = 0;
        ntf.length = htonl(strlen(kick_str));
        
        send(target_fd, &ntf, sizeof(ntf), 0);
        send(target_fd, kick_str, strlen(kick_str), 0);
        
        free(kick_str);
        cJSON_Delete(kick_json);
        
        printf("[Kick Member] Sent NTF_MEMBER_KICKED to target (fd=%d)\n", target_fd);
    }
    
    // ========== BƯỚC 15: BROADCAST NTF_PLAYER_LEFT CHO NGƯỜI CHƠI CÒN LẠI ==========
    // (Target đã bị xóa, nên sẽ không nhận broadcast này)
    cJSON *left_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(left_json, "account_id", target_id);
    char *left_str = cJSON_PrintUnformatted(left_json);
    
    printf("[Kick Member] Broadcasting NTF_PLAYER_LEFT: %s\n", left_str);
    room_broadcast(room_id, NTF_PLAYER_LEFT, left_str, strlen(left_str), -1);
    
    free(left_str);
    cJSON_Delete(left_json);
    
    // ========== BƯỚC 16: BROADCAST NTF_PLAYER_LIST CHO NGƯỜI CHƠI CÒN LẠI ==========
    printf("[Kick Member] Broadcasting NTF_PLAYER_LIST\n");
    broadcast_player_list(room_id);
    
    printf("[Kick Member] ✅ SUCCESS: player %u kicked from room %u by host %u\n",
           target_id, room_id, kicker_id);
}

//==============================================================================
// LEAVE ROOM (Any member)
//==============================================================================
void handle_leave_room(int client_fd, MessageHeader *req, const char *payload) {
    (void)payload; // Empty payload
    printf("[HANDLER] <LEAVE_ROOM> Request from fd=%d\n", client_fd);
    
    // STEP 1: Validate session
    UserSession *session = session_get_by_socket(client_fd);
    if (!session || session->state != SESSION_LOBBY) {
        send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not logged in or not in lobby");
        return;
    }
    
    // STEP 2: Find room
    uint32_t room_id = room_find_by_player_account(session->account_id);
    if (room_id == 0) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Not in any room");
        return;
    }
    
    RoomState *room = room_get_state(room_id);
    if (!room) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Room not found");
        return;
    }
    
    // STEP 3: Validate room status
    if (room->status != ROOM_WAITING) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Cannot leave room during game");
        return;
    }
    
    // LOG BEFORE STATE
    printf("[Leave Room] ===== BEFORE STATE =====\n");
    printf("[Leave Room] [RoomState] Room ID: %u\n", room->id);
    printf("[Leave Room] [RoomState] Host ID: %u\n", room->host_id);
    printf("[Leave Room] [RoomState] Player Count: %d\n", room->player_count);
    printf("[Leave Room] [RoomPlayerState] Players:\n");
    for (int i = 0; i < room->player_count; i++) {
        printf("[Leave Room] [RoomPlayerState]   [%d] %s (id=%u, host=%s, ready=%s, connected=%s)\n",
               i, room->players[i].name, room->players[i].account_id,
               room->players[i].is_host ? "YES" : "NO",
               room->players[i].is_ready ? "YES" : "NO",
               room->players[i].connected ? "YES" : "NO");
    }
    
    uint32_t leaver_id = session->account_id;
    bool was_host = (room->host_id == leaver_id);
    
    printf("[Leave Room] Leaver: %u (was_host=%s)\n", leaver_id, was_host ? "YES" : "NO");
    
    // STEP 4: Remove player from in-memory state
    int removed = room_remove_player(room_id, leaver_id);
    if (removed != 0) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Failed to remove player from room");
        return;
    }
    
    printf("[Leave Room] ✅ Removed player %u from in-memory state\n", leaver_id);
    
    // STEP 5: Remove from DB
    int rc = room_repo_remove_player(room_id, leaver_id);
    if (rc != 0) {
        printf("[Leave Room] ⚠️  Warning: Failed to remove player from DB\n");
        // Continue anyway - in-memory is authoritative
    }
    
    // STEP 6: Host transfer logic
    uint32_t new_host_id = 0;
    bool room_empty = (room->player_count == 0);
    
    if (was_host && !room_empty) {
        printf("[Leave Room] Host is leaving, finding new host...\n");
        printf("[Leave Room] Candidates for new host:\n");
        
        // Log all candidates with their joined_at and connected status
        for (int i = 0; i < room->player_count; i++) {
            printf("[Leave Room]   [%d] %s (id=%u, joined_at=%ld, connected=%s)\n",
                   i, room->players[i].name, room->players[i].account_id,
                   room->players[i].joined_at,
                   room->players[i].connected ? "YES" : "NO");
        }
        
        // Find earliest joiner (min joined_at) WHO IS CONNECTED
        time_t earliest_join = LONG_MAX;
        for (int i = 0; i < room->player_count; i++) {
            // ⚠️ CHỈ XÉT NGƯỜI ĐANG CONNECTED
            if (room->players[i].connected && room->players[i].joined_at < earliest_join) {
                earliest_join = room->players[i].joined_at;
                new_host_id = room->players[i].account_id;
            }
        }
        
        if (new_host_id != 0) {
            printf("[Leave Room] ✅ New host selected: %u (earliest connected joiner, joined_at=%ld)\n", 
                   new_host_id, earliest_join);
        } else {
            printf("[Leave Room] ⚠️  No connected players found for host transfer!\n");
        }
        
        printf("[Leave Room] Host transfer: %u -> %u\n", leaver_id, new_host_id);
        
        // Update in-memory
        room->host_id = new_host_id;
        for (int i = 0; i < room->player_count; i++) {
            room->players[i].is_host = (room->players[i].account_id == new_host_id);
        }
        
        // Update DB
        rc = room_repo_update_host(room_id, new_host_id);
        if (rc != 0) {
            printf("[Leave Room] ⚠️  Warning: Failed to update host in DB\n");
        }
    }
    
    // STEP 7: Close room if empty
    if (room_empty) {
        printf("[Leave Room] Room is empty, closing room %u\n", room_id);
        
        // Close in DB
        rc = room_repo_close_room(room_id);
        if (rc != 0) {
            printf("[Leave Room] ⚠️  Warning: Failed to close room in DB\n");
        }
        
        // Destroy in-memory state
        room_destroy(room_id);
        printf("[Leave Room] ✅ Destroyed in-memory state for room %u\n", room_id);
    }
    
    // LOG AFTER STATE
    if (!room_empty) {
        printf("[Leave Room] ===== AFTER STATE =====\n");
        printf("[Leave Room] [RoomState] Room ID: %u\n", room->id);
        printf("[Leave Room] [RoomState] Host ID: %u (changed=%s)\n", room->host_id, was_host ? "YES" : "NO");
        printf("[Leave Room] [RoomState] Player Count: %d\n", room->player_count);
        printf("[Leave Room] [RoomPlayerState] Players:\n");
        for (int i = 0; i < room->player_count; i++) {
            printf("[Leave Room] [RoomPlayerState]   [%d] %s (id=%u, host=%s, ready=%s, connected=%s)\n",
                   i, room->players[i].name, room->players[i].account_id,
                   room->players[i].is_host ? "YES" : "NO",
                   room->players[i].is_ready ? "YES" : "NO",
                   room->players[i].connected ? "YES" : "NO");
        }
    } else {
        printf("[Leave Room] ===== ROOM CLOSED =====\n");
    }
    
    // STEP 8: Send response to leaver
    forward_response(client_fd, req, RES_ROOM_LEFT, "", 0);
    printf("[Leave Room] Sent RES_ROOM_LEFT to leaver (fd=%d)\n", client_fd);
    
    // STEP 9: Update session state
    session->state = SESSION_LOBBY;
    printf("[Leave Room] Updated session state to SESSION_LOBBY\n");
    
    // STEP 10: Broadcast to remaining players
    if (!room_empty) {
        // Broadcast NTF_PLAYER_LEFT
        cJSON *left_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(left_json, "account_id", leaver_id);
        char *left_str = cJSON_PrintUnformatted(left_json);
        
        printf("[Leave Room] Broadcasting NTF_PLAYER_LEFT: %s\n", left_str);
        room_broadcast(room_id, NTF_PLAYER_LEFT, left_str, strlen(left_str), -1);
        
        free(left_str);
        cJSON_Delete(left_json);
        
        // Broadcast NTF_HOST_CHANGED if host transferred
        if (was_host && new_host_id != 0) {
            cJSON *host_json = cJSON_CreateObject();
            cJSON_AddNumberToObject(host_json, "new_host_id", new_host_id);
            char *host_str = cJSON_PrintUnformatted(host_json);
            
            printf("[Leave Room] Broadcasting NTF_HOST_CHANGED: %s\n", host_str);
            room_broadcast(room_id, NTF_HOST_CHANGED, host_str, strlen(host_str), -1);
            
            free(host_str);
            cJSON_Delete(host_json);
        }
        
        // Broadcast NTF_PLAYER_LIST
        printf("[Leave Room] Broadcasting NTF_PLAYER_LIST\n");
        broadcast_player_list(room_id);
    }
    
    printf("[Leave Room] ✅ SUCCESS: player %u left room %u\n", leaver_id, room_id);
}


//==============================================================================
// GET ROOM LIST
//==============================================================================
void handle_get_room_list(int client_fd, MessageHeader *req, const char *payload) {
    (void)payload; // Request payload is empty/ignored
    
    // 1. Query DB for recent waiting rooms
    // NEW: Query from view 'rooms_with_counts' to get 'current_players'
    
    char query[256];
    snprintf(query, sizeof(query), 
             "SELECT id, name, status, mode, max_players, visibility, wager_mode, current_players FROM rooms_with_counts WHERE status = 'waiting' ORDER BY created_at DESC LIMIT 10");
    
    cJSON *response = NULL;
    // Update target table to 'rooms_with_counts'
    db_error_t rc = db_get("rooms_with_counts", query, &response);
    
    if (rc != DB_OK || !response) {
        printf("[SERVER] [GET_ROOM_LIST] DB query failed\n");
        forward_response(client_fd, req, RES_ROOM_LIST, "[]", 2);
        return;
    }
    
    // 2. Convert to JSON string
    char *json_str = cJSON_PrintUnformatted(response);
    
    if (!json_str) {
        cJSON_Delete(response);
        send_error(client_fd, req, ERR_SERVER_ERROR, "JSON error");
        return;
    }
    
    printf("[SERVER] [GET_ROOM_LIST] Sending %zu bytes\n", strlen(json_str));
    
    // 3. Send response
    forward_response(client_fd, req, RES_ROOM_LIST, json_str, strlen(json_str));
    
    // Cleanup
    free(json_str);
    cJSON_Delete(response);
}
//==============================================================================
// SET GAME RULE
//==============================================================================
void handle_set_rule(int client_fd, MessageHeader *req, const char *payload) {
    printf("[HANDLER] <SET_RULE> Request from fd=%d\n", client_fd);
    
    // STEP 1: Validate session
    UserSession *session = session_get_by_socket(client_fd);
    if (!session || session->state != SESSION_LOBBY) {
        send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not logged in or not in lobby");
        return;
    }
    
    // STEP 2: Parse payload
    if (req->length != 4) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload size");
        return;
    }
    
    uint8_t mode = payload[0];
    uint8_t max_players = payload[1];
    uint8_t visibility = payload[2];
    uint8_t wager_mode = payload[3];
    
    printf("[HANDLER] <SET_RULE> mode=%u, max_players=%u, visibility=%u, wager=%u\n",
           mode, max_players, visibility, wager_mode);
    
    // STEP 3: Find room
    uint32_t room_id = room_find_by_player_account(session->account_id);
    if (room_id == 0) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Not in any room");
        return;
    }
    
    RoomState *room = room_get_state(room_id);
    if (!room) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Room not found");
        return;
    }
    
    // STEP 4: Validate permissions
    if (room->host_id != session->account_id) {
        send_error(client_fd, req, ERR_NOT_HOST, "Only host can change rules");
        return;
    }
    
    if (room->status != ROOM_WAITING) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Cannot change rules after game started");
        return;
    }
    
    // STEP 5: Validate rules
    if (mode == MODE_ELIMINATION && max_players != 4) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Elimination mode requires exactly 4 players");
        return;
    }
    
    if (mode == MODE_SCORING && (max_players < 4 || max_players > 6)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Scoring mode requires 4-6 players");
        return;
    }
    
    // STEP 5.5: Validate max_players vs current players
    if (max_players < room->player_count) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg),
                 "Cannot set max players to %u. Room currently has %d players. "
                 "Please kick players first or choose a higher limit.",
                 max_players, room->player_count);
        send_error(client_fd, req, ERR_BAD_REQUEST, error_msg);
        return;
    }
    
    // Log BEFORE state
    printf("[SetGameRule] BEFORE changes:\n");
    printf("  - room_id: %u\n", room->id);
    printf("  - mode: %u (%s)\n", room->mode, room->mode ? "SCORING" : "ELIMINATION");
    printf("  - max_players: %u\n", room->max_players);
    printf("  - visibility: %u (%s)\n", room->visibility, room->visibility ? "PRIVATE" : "PUBLIC");
    printf("  - wager_mode: %u\n", room->wager_mode);
    printf("  - current_players: %d\n", room->player_count);
    
    // STEP 6: Update in-memory state
    room->mode = mode;
    room->max_players = max_players;
    room->visibility = visibility;
    room->wager_mode = wager_mode;
    
    // Log AFTER state
    printf("[SetGameRule] AFTER changes:\n");
    printf("  - mode: %u (%s)\n", room->mode, room->mode ? "SCORING" : "ELIMINATION");
    printf("  - max_players: %u\n", room->max_players);
    printf("  - visibility: %u (%s)\n", room->visibility, room->visibility ? "PRIVATE" : "PUBLIC");
    printf("  - wager_mode: %u\n", room->wager_mode);
    
    // STEP 7: Reset all ready states
    printf("[SetGameRule] Resetting ready states for %d players:\n", room->player_count);
    for (int i = 0; i < room->player_count; i++) {
        printf("  [%d] %s (id=%u): ready %s -> false\n", 
               i, room->players[i].name, room->players[i].account_id,
               room->players[i].is_ready ? "true" : "false");
        room->players[i].is_ready = false;
    }
    
    // STEP 8: Update database (via repo)
    int rc = room_repo_set_rules(room_id, mode, max_players, visibility, wager_mode);
    if (rc != 0) {
        printf("[SetGameRule] ⚠️  Warning: Failed to sync rules to DB\n");
        // Continue anyway - eventual consistency
    } else {
        printf("[SetGameRule] ✅ DB sync successful\n");
    }
    
    // STEP 9: Send response to host
    forward_response(client_fd, req, RES_RULES_UPDATED, "", 0);
    printf("[SetGameRule] Sent RES_RULES_UPDATED to host (fd=%d)\n", client_fd);
    
    // STEP 10: Broadcast NTF_RULES_CHANGED
    cJSON *rules_json = cJSON_CreateObject();
    cJSON_AddStringToObject(rules_json, "mode", mode ? "scoring" : "elimination");
    cJSON_AddNumberToObject(rules_json, "maxPlayers", max_players);
    cJSON_AddStringToObject(rules_json, "visibility", visibility ? "private" : "public");
    cJSON_AddBoolToObject(rules_json, "wagerMode", wager_mode);
    
    char *rules_str = cJSON_PrintUnformatted(rules_json);
    printf("[SetGameRule] Broadcasting NTF_RULES_CHANGED: %s\n", rules_str);
    room_broadcast(room_id, NTF_RULES_CHANGED, rules_str, strlen(rules_str), -1);
    free(rules_str);
    cJSON_Delete(rules_json);
    
    // STEP 11: Broadcast NTF_PLAYER_LIST (with updated ready states)
    printf("[SetGameRule] Broadcasting NTF_PLAYER_LIST with reset ready states\n");
    broadcast_player_list(room_id);
    
    printf("[SetGameRule] ✅ SUCCESS: room_id=%u, all players notified\n", room_id);
}
//==============================================================================
// READY
//==============================================================================
void handle_ready(int client_fd, MessageHeader *req, const char *payload) {
    printf("[HANDLER] <READY> Request from fd=%d\n", client_fd);
    
    // STEP 1: Validate session
    UserSession *session = session_get_by_socket(client_fd);
    if (!session || session->state != SESSION_LOBBY) {
        send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not logged in or not in lobby");
        return;
    }
    
    // STEP 2: Find room
    uint32_t room_id = room_find_by_player_account(session->account_id);
    if (room_id == 0) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Not in any room");
        return;
    }
    
    RoomState *room = room_get_state(room_id);
    if (!room) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Room not found");
        return;
    }
    
    // STEP 3: Find player in RoomPlayerState array
    RoomPlayerState *player = NULL;
    int player_index = -1;
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].account_id == session->account_id) {
            player = &room->players[i];
            player_index = i;
            break;
        }
    }
    
    if (!player) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Player not found in room");
        return;
    }
    
    // STEP 4: Log BEFORE state
    printf("[Ready] BEFORE toggle:\n");
    printf("  - Player: %s (id=%u, index=%d)\n", player->name, player->account_id, player_index);
    printf("  - Current is_ready: %s\n", player->is_ready ? "true" : "false");
    
    // STEP 5: Toggle ready state in RoomPlayerState
    bool old_state = player->is_ready;
    player->is_ready = !player->is_ready;
    
    printf("[Ready] AFTER toggle:\n");
    printf("  - New is_ready: %s\n", player->is_ready ? "true" : "false");
    printf("  - Change: %s -> %s\n", 
           old_state ? "true" : "false",
           player->is_ready ? "true" : "false");
    
    // STEP 6: Log ALL players' ready status
    printf("[Ready] Current ready status of ALL players in room %u:\n", room_id);
    int ready_count = 0;
    for (int i = 0; i < room->player_count; i++) {
        printf("  [%d] %s (id=%u): %s%s\n",
               i,
               room->players[i].name,
               room->players[i].account_id,
               room->players[i].is_ready ? "✅ READY" : "❌ NOT READY",
               room->players[i].is_host ? " (HOST)" : "");
        if (room->players[i].is_ready) ready_count++;
    }
    printf("[Ready] Summary: %d/%d players ready\n", ready_count, room->player_count);
    
    // STEP 7: Send response (empty payload)
    forward_response(client_fd, req, RES_READY_OK, "", 0);
    
    // STEP 8: Broadcast NTF_PLAYER_READY
    cJSON *notif = cJSON_CreateObject();
    cJSON_AddNumberToObject(notif, "account_id", session->account_id);
    cJSON_AddBoolToObject(notif, "is_ready", player->is_ready);
    
    char *notif_str = cJSON_PrintUnformatted(notif);
    printf("[Ready] Broadcasting NTF_PLAYER_READY: %s\n", notif_str);
    room_broadcast(room_id, NTF_PLAYER_READY, notif_str, strlen(notif_str), -1);
    free(notif_str);
    cJSON_Delete(notif);
    
    printf("[Ready] ✅ SUCCESS: room_id=%u, account_id=%u, is_ready=%s\n",
           room_id, session->account_id, player->is_ready ? "true" : "false");
}
