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
static void send_error(int client_fd, MessageHeader *req, const char *message) {
    // Build JSON error response once - no double encoding
    char error_json[256];
    snprintf(error_json, sizeof(error_json), 
        "{\"success\":false,\"error\":\"%s\"}", message);
    
    // Always use RES_GAME_STARTED for consistency with frontend handler
    forward_response(client_fd, req, RES_GAME_STARTED, error_json, strlen(error_json));
}

//==============================================================================
// START GAME
//==============================================================================
void handle_start_game(int client_fd, MessageHeader *req, const char *payload) {
    // 1. Validate payload
    if (req->length != sizeof(StartGamePayload)) {
        send_error(client_fd, req, "Invalid payload");
        return;
    }
    
    // 2. Parse room_id
    StartGamePayload data;
    memcpy(&data, payload, sizeof(data));
    uint32_t room_id = ntohl(data.room_id);
    
    printf("[MATCH] START_GAME request for room_id=%u\n", room_id);
    
    // 3. Check player count
    int total_count = room_get_member_count(room_id);
    
    printf("[MATCH] room_get_member_count(%u) = %d\n", room_id, total_count);
    
    if (total_count < 4 || total_count > 6) {
        char msg[128];
        snprintf(msg, sizeof(msg),
            "Need 4-6 players (current: %d)", 
            total_count);
        send_error(client_fd, req, msg);
        return;
    }
    
    // 4. Check all players ready (MEMORY)
    int ready_count = 0;
    int player_count = 0;
    
    if (!room_all_ready(room_id, &ready_count, &player_count)) {
        char msg[128];
        snprintf(msg, sizeof(msg),
            "Not all ready (%d/%d)", 
            ready_count, player_count);
        send_error(client_fd, req, msg);
        return;
    }
    
    // 5. Success - broadcast game start
    char success_json[256];
    snprintf(success_json, sizeof(success_json),
        "{\"success\":true,\"room_id\":%u,\"player_count\":%d,\"message\":\"Game starting...\"}", 
        room_id, player_count);
    
    printf("[MATCH] Starting game for room=%u with %d players\n", room_id, player_count);
    
    // Broadcast to all members
    room_broadcast(room_id, NTF_GAME_START, success_json, strlen(success_json), -1);
    
    // Response to host
    forward_response(client_fd, req, RES_GAME_STARTED, success_json, strlen(success_json));
}
