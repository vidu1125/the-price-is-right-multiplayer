#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "handlers/presence_handler.h"
#include "handlers/session_context.h"
#include "handlers/presence_manager.h"
#include "protocol/protocol.h"
#include "protocol/opcode.h"

// ============================================================================
// HELPERS
// ============================================================================

static void send_response(
    int client_fd,
    MessageHeader *req,
    uint16_t response_code,
    cJSON *payload
) {
    char *json_str = cJSON_PrintUnformatted(payload);
    if (!json_str) {
        printf("[PRESENCE] Failed to serialize JSON\n");
        return;
    }

    forward_response(client_fd, req, response_code, json_str, strlen(json_str));
    free(json_str);
}

static void send_error(
    int client_fd,
    MessageHeader *req,
    uint16_t error_code,
    const char *message
) {
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddBoolToObject(payload, "success", false);
    cJSON_AddStringToObject(payload, "error", message);

    send_response(client_fd, req, error_code, payload);
    cJSON_Delete(payload);
}

static int32_t get_client_account_id(int client_fd) {
    return get_client_account(client_fd);
}

// ============================================================================
// STATUS UPDATE
// ============================================================================

void handle_status_update(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[PRESENCE] ========== STATUS UPDATE REQUEST ==========\n");

    int32_t account_id = get_client_account_id(client_fd);
    if (account_id <= 0) {
        printf("[PRESENCE] User not logged in\n");
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }

    // Parse JSON
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        printf("[PRESENCE] Failed to parse JSON\n");
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *status_json = cJSON_GetObjectItem(json, "status");
    cJSON *room_id_json = cJSON_GetObjectItem(json, "room_id");

    if (!status_json || !cJSON_IsNumber(status_json)) {
        printf("[PRESENCE] Missing or invalid status\n");
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing status");
        return;
    }

    int status_val = status_json->valueint;
    int32_t room_id = 0;

    // Validate status value
    if (status_val < 0 || status_val > 2) {
        printf("[PRESENCE] Invalid status value: %d\n", status_val);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid status value");
        return;
    }

    presence_status_t new_status = (presence_status_t)status_val;

    // Get room_id if provided and status is PLAYING
    if (new_status == PRESENCE_PLAYING) {
        if (room_id_json && cJSON_IsNumber(room_id_json)) {
            room_id = room_id_json->valueint;
        }
        
        // Validate room_id for PLAYING status
        if (room_id <= 0) {
            printf("[PRESENCE] PLAYING status requires room_id > 0\n");
            cJSON_Delete(json);
            send_error(client_fd, header, ERR_BAD_REQUEST, "room_id required for PLAYING status");
            return;
        }
    }

    cJSON_Delete(json);

    // Update presence
    presence_update_status(account_id, new_status, room_id);

    // Success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "status", (int)new_status);
    if (room_id > 0) {
        cJSON_AddNumberToObject(response, "room_id", room_id);
    }
    cJSON_AddStringToObject(response, "message", "Status updated");

    send_response(client_fd, header, RES_STATUS_UPDATED, response);
    cJSON_Delete(response);

    // Log status change
    const char *status_name = new_status == PRESENCE_OFFLINE ? "OFFLINE" :
                              new_status == PRESENCE_ONLINE_IDLE ? "ONLINE_IDLE" :
                              "PLAYING";
    
    if (room_id > 0) {
        printf("[PRESENCE] Status changed: account=%d, status=%s, room=%d\n", 
               account_id, status_name, room_id);
    } else {
        printf("[PRESENCE] Status changed: account=%d, status=%s\n", 
               account_id, status_name);
    }
}

// ============================================================================
// GET FRIEND STATUS
// ============================================================================

void handle_get_friend_status(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[PRESENCE] ========== GET FRIEND STATUS REQUEST ==========\n");

    int32_t account_id = get_client_account_id(client_fd);
    if (account_id <= 0) {
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }

    // Parse JSON
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *friend_id_json = cJSON_GetObjectItem(json, "friend_id");
    if (!friend_id_json || !cJSON_IsNumber(friend_id_json)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing friend_id");
        return;
    }

    int32_t friend_id = friend_id_json->valueint;
    cJSON_Delete(json);

    if (friend_id <= 0) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid friend_id");
        return;
    }

    // Get friend's presence status
    presence_status_t friend_status = presence_get_status(friend_id);
    int32_t room_id = 0;

    // Get room_id if friend is playing
    if (friend_status == PRESENCE_PLAYING) {
        const online_user_t *friend_user = presence_get_user(friend_id);
        if (friend_user) {
            room_id = friend_user->current_room_id;
        }
    }

    // Success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "friend_id", friend_id);
    cJSON_AddNumberToObject(response, "status", (int)friend_status);
    
    if (room_id > 0) {
        cJSON_AddNumberToObject(response, "room_id", room_id);
    }

    send_response(client_fd, header, RES_SUCCESS, response);
    cJSON_Delete(response);

    printf("[PRESENCE] Friend status retrieved: friend=%d, status=%d\n", friend_id, (int)friend_status);
}
