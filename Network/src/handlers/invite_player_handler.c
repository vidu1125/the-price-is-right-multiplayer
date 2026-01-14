#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <sys/socket.h>
#include "handlers/invite_player_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "handlers/session_manager.h"
#include "transport/room_manager.h"

#if defined(__GNUC__) || defined(__clang__)
    #define PACKED __attribute__((packed))
#else
    #define PACKED
    #pragma pack(push, 1)
#endif

typedef struct PACKED {
    int32_t target_id;
    uint32_t room_id;
} InvitePlayerPayload;

#if !defined(__GNUC__) && !defined(__clang__)
    #pragma pack(pop)
#endif

static void send_error(int client_fd, MessageHeader *req, uint16_t error_code, const char *message) {
    forward_response(client_fd, req, error_code, message, strlen(message));
}

/**
 * Internal helper to send a notification to a specific client
 */
static void send_notification_to_fd(int client_fd, uint16_t command, const char *payload, uint32_t payload_len) {
    MessageHeader header;
    memset(&header, 0, sizeof(header));
    header.magic = htons(MAGIC_NUMBER);
    header.version = PROTOCOL_VERSION;
    header.command = htons(command);
    header.seq_num = 0;
    header.length = htonl(payload_len);
    
    send(client_fd, &header, sizeof(header), 0);
    if (payload_len > 0 && payload) {
        send(client_fd, payload, payload_len, 0);
    }
}

void handle_invite_player(int client_fd, MessageHeader *req, const char *payload) {
    printf("[SERVER] [INVITE_PLAYER] Request from fd=%d\n", client_fd);

    // 1. Validate session
    UserSession *sender_session = session_get_by_socket(client_fd);
    if (!sender_session || sender_session->state == SESSION_UNAUTHENTICATED) {
        printf("[SERVER] [INVITE_PLAYER] Error: Not logged in (fd=%d)\n", client_fd);
        send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }

    // 2. Validate payload size
    if (req->length != sizeof(InvitePlayerPayload)) {
        printf("[SERVER] [INVITE_PLAYER] Error: Invalid payload size %u, expected %lu\n", 
               req->length, sizeof(InvitePlayerPayload));
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload size");
        return;
    }

    // 3. Parse payload
    InvitePlayerPayload data;
    memcpy(&data, payload, sizeof(data));
    int32_t target_id = ntohl(data.target_id);
    uint32_t room_id = ntohl(data.room_id);

    printf("[SERVER] [INVITE_PLAYER] Sender: %d, Target: %d, Room: %u\n", sender_session->account_id, target_id, room_id);

    // 4. Validate room existence
    RoomState *room = room_get(room_id);
    if (!room) {
        printf("[SERVER] [INVITE_PLAYER] Error: Room %u not found\n", room_id);
        send_error(client_fd, req, ERR_BAD_REQUEST, "Room not found");
        return;
    }

    // 5. Check target player session
    UserSession *target_session = session_get_by_account(target_id);

    if (!target_session) {
        printf("[SERVER] [INVITE_PLAYER] Error: Target %d not found or disconnected\n", target_id);
        send_error(client_fd, req, ERR_BAD_REQUEST, "The requested user is currently disconnected or does not exist.");
        return;
    }

    // Check if target is in LOBBY state
    if (target_session->state != SESSION_LOBBY) {
        printf("[SERVER] [INVITE_PLAYER] Error: Target %d in state %d (busy)\n", target_id, target_session->state);
        send_error(client_fd, req, ERR_BAD_REQUEST, "The requested user is currently in a game or busy and cannot receive invitations.");
        return;
    }

    // 6. Construct invitation notification
    cJSON *notif_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(notif_json, "sender_id", sender_session->account_id);
    cJSON_AddNumberToObject(notif_json, "room_id", room_id);
    cJSON_AddStringToObject(notif_json, "room_name", room->name);
    
    char *json_str = cJSON_PrintUnformatted(notif_json);
    uint32_t json_len = strlen(json_str);

    // 7. Send notification to target
    send_notification_to_fd(target_session->socket_fd, NTF_INVITATION, json_str, json_len);
    printf("[SERVER] [INVITE_PLAYER] Notification (NTF_INVITATION) sent to account %d (fd=%d)\n", 
           target_id, target_session->socket_fd);

    // 8. Respond to sender (success)
    forward_response(client_fd, req, RES_SUCCESS, "Invitation sent", 15);

    // Cleanup
    free(json_str);
    cJSON_Delete(notif_json);

    printf("[SERVER] [INVITE_PLAYER] ✅ SUCCESS: Invitation processed\n");
}
