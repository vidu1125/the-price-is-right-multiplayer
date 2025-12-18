// Network/server/src/router.c
/**
 * Router - Forward commands to Python Backend via IPC
 * This is a THIN layer - only handles protocol translation
 */
#include "router.h"
#include "session.h"
#include "backend_bridge.h"
#include "packet_handler.h"
#include "../../protocol/commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

// Simple JSON parser helper (for response only)
#include <cjson/cJSON.h>

// ========================================
// HELPER: Extract boolean from JSON
// ========================================
static int json_get_bool(cJSON* json, const char* key, int default_val) {
    cJSON* item = cJSON_GetObjectItem(json, key);
    return (item && cJSON_IsBool(item)) ? cJSON_IsTrue(item) : default_val;
}

// ========================================
// HELPER: Extract integer from JSON
// ========================================
static int json_get_int(cJSON* json, const char* key, int default_val) {
    cJSON* item = cJSON_GetObjectItem(json, key);
    return (item && cJSON_IsNumber(item)) ? item->valueint : default_val;
}

// ========================================
// HELPER: Extract string from JSON
// ========================================
static const char* json_get_string(cJSON* json, const char* key) {
    cJSON* item = cJSON_GetObjectItem(json, key);
    return (item && cJSON_IsString(item)) ? item->valuestring : NULL;
}

// ========================================
// 1. CREATE ROOM
// ========================================
void route_create_room(int client_fd, CreateRoomRequest* req) {
    SessionContext* ctx = session_get(client_fd);
    if (!ctx) {
        send_error(client_fd, ERR_SERVER_ERROR, "Session not found");
        return;
    }
    
    // FSM check
    if (ctx->state != STATE_LOBBY) {
        send_error(client_fd, ERR_BAD_REQUEST, "Must be in lobby");
        return;
    }
    
    // Build data JSON
    char data_json[512];
    snprintf(data_json, sizeof(data_json),
        "{\"name\":\"%s\",\"visibility\":\"%s\",\"mode\":\"%s\","
        "\"max_players\":%u,\"round_time\":%u,\"wager_enabled\":%s}",
        req->room_name,
        req->visibility == 0 ? "public" : "private",
        req->mode == 0 ? "scoring" : "elimination",
        req->max_players,
        ntohs(req->round_time),
        req->wager_enabled ? "true" : "false"
    );
    
    // Build request
    char* request = backend_bridge_build_request("CREATE_ROOM", ctx->user_id, data_json);
    if (!request) {
        send_error(client_fd, ERR_SERVER_ERROR, "Failed to build request");
        return;
    }
    
    // Send to backend
    char* response_str = backend_bridge_send(request);
    free(request);
    
    if (!response_str) {
        send_error(client_fd, ERR_SERVER_ERROR, "Backend connection error");
        return;
    }
    
    // Parse response
    cJSON* response = cJSON_Parse(response_str);
    free(response_str);
    
    if (!response) {
        send_error(client_fd, ERR_SERVER_ERROR, "Invalid backend response");
        return;
    }
    
    if (json_get_bool(response, "success", 0)) {
        // Update session state
        ctx->state = STATE_IN_ROOM;
        ctx->room_id = json_get_int(response, "room_id", 0);
        ctx->is_host = true;
        
        // Build binary response
        CreateRoomResponse res;
        res.room_id = htonl(ctx->room_id);
        res.host_id = htonl(ctx->user_id);
        res.created_at = htobe64(json_get_int(response, "timestamp", 0));
        
        const char* code = json_get_string(response, "room_code");
        if (code) {
            strncpy(res.room_code, code, 10);
            res.room_code[10] = '\0';
        }
        
        send_response(client_fd, RES_ROOM_CREATED, &res, sizeof(res));
        
        printf("[Router] Room created: room_id=%u by user_id=%u\n", ctx->room_id, ctx->user_id);
    } else {
        const char* error = json_get_string(response, "error");
        send_error(client_fd, ERR_BAD_REQUEST, error ? error : "Unknown error");
    }
    
    cJSON_Delete(response);
}

// ========================================
// 2. SET RULE
// ========================================
void route_set_rule(int client_fd, SetRuleRequest* req) {
    SessionContext* ctx = session_get(client_fd);
    if (!ctx) {
        send_error(client_fd, ERR_SERVER_ERROR, "Session not found");
        return;
    }
    
    if (!ctx->is_host) {
        send_error(client_fd, ERR_NOT_HOST, "Only host can change rules");
        return;
    }
    
    // Build data JSON
    char data_json[256];
    snprintf(data_json, sizeof(data_json),
        "{\"room_id\":%u,\"mode\":\"%s\",\"max_players\":%u,"
        "\"round_time\":%u,\"wager_enabled\":%s}",
        ntohl(req->room_id),
        req->mode == 0 ? "scoring" : "elimination",
        req->max_players,
        ntohs(req->round_time),
        req->wager_enabled ? "true" : "false"
    );
    
    char* request = backend_bridge_build_request("SET_RULE", ctx->user_id, data_json);
    char* response_str = backend_bridge_send(request);
    free(request);
    
    if (!response_str) {
        send_error(client_fd, ERR_SERVER_ERROR, "Backend error");
        return;
    }
    
    cJSON* response = cJSON_Parse(response_str);
    free(response_str);
    
    if (json_get_bool(response, "success", 0)) {
        send_response(client_fd, RES_SUCCESS, NULL, 0);
        
        // TODO: Broadcast NTF_RULES_UPDATED to room members
        printf("[Router] Rules updated for room_id=%u\n", ntohl(req->room_id));
    } else {
        const char* error = json_get_string(response, "error");
        send_error(client_fd, ERR_BAD_REQUEST, error ? error : "Failed to update rules");
    }
    
    cJSON_Delete(response);
}

// ========================================
// 3. KICK MEMBER
// ========================================
void route_kick_member(int client_fd, KickRequest* req) {
    SessionContext* ctx = session_get(client_fd);
    if (!ctx) {
        send_error(client_fd, ERR_SERVER_ERROR, "Session not found");
        return;
    }
    
    if (!ctx->is_host) {
        send_error(client_fd, ERR_NOT_HOST, "Only host can kick");
        return;
    }
    
    // Build data JSON
    char data_json[128];
    snprintf(data_json, sizeof(data_json),
        "{\"room_id\":%u,\"target_user_id\":%u}",
        ntohl(req->room_id),
        ntohl(req->target_user_id)
    );
    
    char* request = backend_bridge_build_request("KICK_MEMBER", ctx->user_id, data_json);
    char* response_str = backend_bridge_send(request);
    free(request);
    
    if (!response_str) {
        send_error(client_fd, ERR_SERVER_ERROR, "Backend error");
        return;
    }
    
    cJSON* response = cJSON_Parse(response_str);
    free(response_str);
    
    if (json_get_bool(response, "success", 0)) {
        send_response(client_fd, RES_SUCCESS, NULL, 0);
        
        // Get target user session and notify
        uint32_t target_id = ntohl(req->target_user_id);
        SessionContext* target_ctx = session_get_by_user_id(target_id);
        
        if (target_ctx) {
            KickedNotification ntf;
            ntf.room_id = req->room_id;
            ntf.kicked_at = htobe64(json_get_int(response, "timestamp", 0));
            strcpy(ntf.reason, "You have been kicked by the host");
            
            send_notification(target_ctx->socket_fd, NTF_KICKED, &ntf, sizeof(ntf));
            
            // Update target session
            target_ctx->state = STATE_LOBBY;
            target_ctx->room_id = 0;
        }
        
        printf("[Router] User %u kicked from room %u\n", target_id, ntohl(req->room_id));
    } else {
        const char* error = json_get_string(response, "error");
        send_error(client_fd, ERR_BAD_REQUEST, error ? error : "Failed to kick member");
    }
    
    cJSON_Delete(response);
}

// ========================================
// 4. DELETE ROOM
// ========================================
void route_delete_room(int client_fd, DeleteRoomRequest* req) {
    SessionContext* ctx = session_get(client_fd);
    if (!ctx) {
        send_error(client_fd, ERR_SERVER_ERROR, "Session not found");
        return;
    }
    
    if (!ctx->is_host) {
        send_error(client_fd, ERR_NOT_HOST, "Only host can delete room");
        return;
    }
    
    // Build data JSON
    char data_json[64];
    snprintf(data_json, sizeof(data_json), "{\"room_id\":%u}", ntohl(req->room_id));
    
    char* request = backend_bridge_build_request("DELETE_ROOM", ctx->user_id, data_json);
    char* response_str = backend_bridge_send(request);
    free(request);
    
    if (!response_str) {
        send_error(client_fd, ERR_SERVER_ERROR, "Backend error");
        return;
    }
    
    cJSON* response = cJSON_Parse(response_str);
    free(response_str);
    
    if (json_get_bool(response, "success", 0)) {
        send_response(client_fd, RES_SUCCESS, NULL, 0);
        
        // Notify all members (TODO: Implement broadcast)
        
        // Update host session
        ctx->state = STATE_LOBBY;
        ctx->room_id = 0;
        ctx->is_host = false;
        
        printf("[Router] Room %u deleted\n", ntohl(req->room_id));
    } else {
        const char* error = json_get_string(response, "error");
        send_error(client_fd, ERR_BAD_REQUEST, error ? error : "Failed to delete room");
    }
    
    cJSON_Delete(response);
}

// ========================================
// 5. START GAME
// ========================================
void route_start_game(int client_fd, StartGameRequest* req) {
    SessionContext* ctx = session_get(client_fd);
    if (!ctx) {
        send_error(client_fd, ERR_SERVER_ERROR, "Session not found");
        return;
    }
    
    if (!ctx->is_host) {
        send_error(client_fd, ERR_NOT_HOST, "Only host can start game");
        return;
    }
    
    // Build data JSON
    char data_json[64];
    snprintf(data_json, sizeof(data_json), "{\"room_id\":%u}", ntohl(req->room_id));
    
    char* request = backend_bridge_build_request("START_GAME", ctx->user_id, data_json);
    char* response_str = backend_bridge_send(request);
    free(request);
    
    if (!response_str) {
        send_error(client_fd, ERR_SERVER_ERROR, "Backend error");
        return;
    }
    
    cJSON* response = cJSON_Parse(response_str);
    free(response_str);
    
    if (json_get_bool(response, "success", 0)) {
        // Update session
        ctx->state = STATE_PLAYING;
        ctx->match_id = json_get_int(response, "match_id", 0);
        
        send_response(client_fd, RES_SUCCESS, NULL, 0);
        
        // Send countdown notification
        GameStartingNotification ntf;
        ntf.match_id = htonl(ctx->match_id);
        ntf.countdown = 3;
        ntf.server_timestamp = htobe64(json_get_int(response, "server_timestamp", 0));
        
        send_notification(client_fd, NTF_GAME_STARTING, &ntf, sizeof(ntf));
        
        printf("[Router] Game started: match_id=%u\n", ctx->match_id);
        
        // TODO: Broadcast to all room members
        // TODO: Send round start notification
    } else {
        const char* error = json_get_string(response, "error");
        send_error(client_fd, ERR_BAD_REQUEST, error ? error : "Failed to start game");
    }
    
    cJSON_Delete(response);
}

// ========================================
// MAIN ROUTER
// ========================================
void route_command(uint16_t command, int client_fd, void* payload, uint32_t length) {
    switch (command) {
        case CMD_CREATE_ROOM:
            if (length >= sizeof(CreateRoomRequest)) {
                route_create_room(client_fd, (CreateRoomRequest*)payload);
            } else {
                send_error(client_fd, ERR_BAD_REQUEST, "Invalid payload size");
            }
            break;
        
        case CMD_SET_RULE:
            if (length >= sizeof(SetRuleRequest)) {
                route_set_rule(client_fd, (SetRuleRequest*)payload);
            } else {
                send_error(client_fd, ERR_BAD_REQUEST, "Invalid payload size");
            }
            break;
        
        case CMD_KICK:
            if (length >= sizeof(KickRequest)) {
                route_kick_member(client_fd, (KickRequest*)payload);
            } else {
                send_error(client_fd, ERR_BAD_REQUEST, "Invalid payload size");
            }
            break;
        
        case CMD_DELETE_ROOM:
            if (length >= sizeof(DeleteRoomRequest)) {
                route_delete_room(client_fd, (DeleteRoomRequest*)payload);
            } else {
                send_error(client_fd, ERR_BAD_REQUEST, "Invalid payload size");
            }
            break;
        
        case CMD_START_GAME:
            if (length >= sizeof(StartGameRequest)) {
                route_start_game(client_fd, (StartGameRequest*)payload);
            } else {
                send_error(client_fd, ERR_BAD_REQUEST, "Invalid payload size");
            }
            break;
        
        default:
            printf("[Router] Unknown command: 0x%04x\n", command);
            send_error(client_fd, ERR_BAD_REQUEST, "Unknown command");
    }
}
