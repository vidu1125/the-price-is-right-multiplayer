#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "handlers/social_handler.h"
#include "handlers/session_context.h"
#include "handlers/presence_manager.h"
#include "db/repo/friend_repo.h"
#include "db/repo/account_repo.h"
#include "db/repo/profile_repo.h"
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
        printf("[SOCIAL] Failed to serialize JSON\n");
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
// FRIEND ADD
// ============================================================================

void handle_friend_add(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[SOCIAL] ========== FRIEND ADD REQUEST ==========\n");

    // Check authentication
    int32_t account_id = get_client_account_id(client_fd);
    if (account_id <= 0) {
        printf("[SOCIAL] User not logged in\n");
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }

    // Parse JSON
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        printf("[SOCIAL] Failed to parse JSON\n");
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *target_id_json = cJSON_GetObjectItem(json, "target_id");
    cJSON *target_username_json = cJSON_GetObjectItem(json, "target_username");

    int32_t target_id = 0;
    if (target_id_json && cJSON_IsNumber(target_id_json)) {
        target_id = target_id_json->valueint;
    } else if (target_username_json && cJSON_IsString(target_username_json)) {
        // TODO: Lookup by username (requires profile lookup)
        printf("[SOCIAL] Looking up target by username\n");
        // For now, error
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Username lookup not implemented");
        return;
    } else {
        printf("[SOCIAL] Missing target_id or target_username\n");
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing target_id or target_username");
        return;
    }

    if (target_id <= 0) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid target_id");
        return;
    }

    // Check self-add
    if (target_id == account_id) {
        printf("[SOCIAL] User trying to add themselves\n");
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_SELF_ADD, "Cannot add yourself");
        return;
    }

    // Check if target exists
    account_t *target_account = NULL;
    db_error_t err = account_find_by_id(target_id, &target_account);
    if (err != DB_SUCCESS || !target_account) {
        printf("[SOCIAL] Target user not found: %d\n", target_id);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_FRIEND_NOT_FOUND, "User not found");
        return;
    }

    account_free(target_account);

    // Check if already friends
    bool are_friends = false;
    err = friend_check_relationship(account_id, target_id, &are_friends);
    if (are_friends) {
        printf("[SOCIAL] Already friends with %d\n", target_id);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_ALREADY_FRIENDS, "Already friends");
        return;
    }

    // Create friend request
    friend_request_t *req_obj = NULL;
    err = friend_request_create(account_id, target_id, &req_obj);

    if (err == DB_ERR_CONFLICT) {
        printf("[SOCIAL] Friend request already exists\n");
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_FRIEND_REQ_DUPLICATE, "Friend request already sent");
        return;
    }

    if (err != DB_SUCCESS || !req_obj) {
        printf("[SOCIAL] Error creating friend request: %d\n", err);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to create friend request");
        return;
    }

    cJSON_Delete(json);

    // Success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "request_id", req_obj->id);
    cJSON_AddNumberToObject(response, "receiver_id", target_id);
    cJSON_AddStringToObject(response, "message", "Friend request sent");

    send_response(client_fd, header, RES_FRIEND_ADDED, response);
    cJSON_Delete(response);

    friend_request_free(req_obj);

    // Notify target if online
    const online_user_t *target_user = presence_get_user(target_id);
    if (target_user && target_user->client_fd > 0) {
        printf("[SOCIAL] Notifying target user about friend request\n");
        
        // Get requester's profile for notification
        profile_t *profile = NULL;
        err = profile_find_by_account(account_id, &profile);

        cJSON *ntf_payload = cJSON_CreateObject();
        cJSON_AddNumberToObject(ntf_payload, "request_id", req_obj->id);
        cJSON_AddNumberToObject(ntf_payload, "sender_id", account_id);
        if (profile && profile->name) {
            cJSON_AddStringToObject(ntf_payload, "sender_name", profile->name);
        }
        if (profile && profile->avatar) {
            cJSON_AddStringToObject(ntf_payload, "sender_avatar", profile->avatar);
        }

        char *ntf_json = cJSON_PrintUnformatted(ntf_payload);
        if (ntf_json) {
            forward_response(
                target_user->client_fd,
                header,
                NTF_FRIEND_REQUEST,
                ntf_json,
                strlen(ntf_json)
            );
            free(ntf_json);
        }

        cJSON_Delete(ntf_payload);
        if (profile) profile_free(profile);
    }

    printf("[SOCIAL] Friend request created successfully\n");
}

// ============================================================================
// FRIEND ACCEPT
// ============================================================================

void handle_friend_accept(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[SOCIAL] ========== FRIEND ACCEPT REQUEST ==========\n");

    int32_t account_id = get_client_account_id(client_fd);
    if (account_id <= 0) {
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }

    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *req_id_json = cJSON_GetObjectItem(json, "request_id");
    if (!req_id_json || !cJSON_IsNumber(req_id_json)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing request_id");
        return;
    }

    int32_t request_id = req_id_json->valueint;

    // Accept request
    friend_request_t *accepted_req = NULL;
    db_error_t err = friend_request_accept(request_id, &accepted_req);

    if (err != DB_SUCCESS || !accepted_req) {
        printf("[SOCIAL] Error accepting request: %d\n", err);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_FRIEND_REQ_NOT_FOUND, "Request not found");
        return;
    }

    int32_t sender_id = accepted_req->sender_id;
    friend_request_free(accepted_req);
    cJSON_Delete(json);

    // TODO: Create bidirectional friendship records

    // Success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "friend_id", sender_id);
    cJSON_AddStringToObject(response, "message", "Friend request accepted");

    send_response(client_fd, header, RES_FRIEND_REQUEST_ACCEPTED, response);
    cJSON_Delete(response);

    // Notify sender if online
    const online_user_t *sender_user = presence_get_user(sender_id);
    if (sender_user && sender_user->client_fd > 0) {
        cJSON *ntf = cJSON_CreateObject();
        cJSON_AddNumberToObject(ntf, "friend_id", account_id);
        cJSON_AddStringToObject(ntf, "message", "Friend request accepted");

        char *ntf_json = cJSON_PrintUnformatted(ntf);
        if (ntf_json) {
            forward_response(sender_user->client_fd, header, NTF_FRIEND_ACCEPTED, ntf_json, strlen(ntf_json));
            free(ntf_json);
        }
        cJSON_Delete(ntf);
    }

    printf("[SOCIAL] Friend request accepted\n");
}

// ============================================================================
// FRIEND REJECT
// ============================================================================

void handle_friend_reject(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[SOCIAL] ========== FRIEND REJECT REQUEST ==========\n");

    int32_t account_id = get_client_account_id(client_fd);
    if (account_id <= 0) {
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }

    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *req_id_json = cJSON_GetObjectItem(json, "request_id");
    if (!req_id_json || !cJSON_IsNumber(req_id_json)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing request_id");
        return;
    }

    int32_t request_id = req_id_json->valueint;

    // Reject request
    friend_request_t *rejected_req = NULL;
    db_error_t err = friend_request_reject(request_id, &rejected_req);

    if (err != DB_SUCCESS || !rejected_req) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_FRIEND_REQ_NOT_FOUND, "Request not found");
        return;
    }

    friend_request_free(rejected_req);
    cJSON_Delete(json);

    // Success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Friend request rejected");

    send_response(client_fd, header, RES_FRIEND_REQUEST_REJECTED, response);
    cJSON_Delete(response);

    printf("[SOCIAL] Friend request rejected\n");
}

// ============================================================================
// FRIEND REMOVE
// ============================================================================

void handle_friend_remove(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[SOCIAL] ========== FRIEND REMOVE REQUEST ==========\n");

    int32_t account_id = get_client_account_id(client_fd);
    if (account_id <= 0) {
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }

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

    // Remove friendship
    db_error_t err = friend_remove(account_id, friend_id);

    if (err != DB_SUCCESS) {
        printf("[SOCIAL] Error removing friend: %d\n", err);
        send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to remove friend");
        return;
    }

    // Success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "friend_id", friend_id);
    cJSON_AddStringToObject(response, "message", "Friend removed");

    send_response(client_fd, header, RES_FRIEND_REMOVED, response);
    cJSON_Delete(response);

    printf("[SOCIAL] Friend removed successfully\n");
}

// ============================================================================
// FRIEND LIST
// ============================================================================

void handle_friend_list(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[SOCIAL] ========== FRIEND LIST REQUEST ==========\n");
    (void)payload;

    int32_t account_id = get_client_account_id(client_fd);
    if (account_id <= 0) {
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }

    // Get friends from presence manager
    int32_t friend_count = 0;
    friend_presence_t *friends = presence_get_friends_status(account_id, &friend_count);

    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);

    cJSON *friends_array = cJSON_CreateArray();

    for (int32_t i = 0; i < friend_count; i++) {
        cJSON *friend_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(friend_obj, "friend_id", friends[i].friend_id);
        cJSON_AddNumberToObject(friend_obj, "status", (int)friends[i].status);
        
        if (friends[i].status == PRESENCE_PLAYING && friends[i].current_room_id > 0) {
            cJSON_AddNumberToObject(friend_obj, "room_id", friends[i].current_room_id);
        }

        cJSON_AddItemToArray(friends_array, friend_obj);
    }

    cJSON_AddItemToObject(response, "friends", friends_array);

    send_response(client_fd, header, RES_FRIEND_LIST, response);
    cJSON_Delete(response);

    if (friends) free(friends);

    printf("[SOCIAL] Friend list sent (%d friends)\n", friend_count);
}

// ============================================================================
// FRIEND REQUESTS
// ============================================================================

void handle_friend_requests(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[SOCIAL] ========== FRIEND REQUESTS LIST ==========\n");
    (void)payload;

    int32_t account_id = get_client_account_id(client_fd);
    if (account_id <= 0) {
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }

    // Get pending requests from DB
    friend_request_t **requests = NULL;
    int32_t request_count = 0;

    db_error_t err = friend_request_list(account_id, &requests, &request_count);

    if (err != DB_SUCCESS) {
        printf("[SOCIAL] Error fetching requests: %d\n", err);
        send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to fetch requests");
        return;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);

    cJSON *requests_array = cJSON_CreateArray();

    for (int32_t i = 0; i < request_count; i++) {
        friend_request_t *req = requests[i];

        // Get sender profile for additional details
        profile_t *sender_profile = NULL;
        profile_find_by_account(req->sender_id, &sender_profile);

        cJSON *req_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(req_obj, "request_id", req->id);
        cJSON_AddNumberToObject(req_obj, "sender_id", req->sender_id);

        if (sender_profile && sender_profile->name) {
            cJSON_AddStringToObject(req_obj, "sender_name", sender_profile->name);
        }
        if (sender_profile && sender_profile->avatar) {
            cJSON_AddStringToObject(req_obj, "sender_avatar", sender_profile->avatar);
        }

        cJSON_AddItemToArray(requests_array, req_obj);

        if (sender_profile) profile_free(sender_profile);
    }

    cJSON_AddItemToObject(response, "requests", requests_array);

    send_response(client_fd, header, RES_FRIEND_REQUESTS, response);
    cJSON_Delete(response);

    if (requests) friend_request_free_array(requests, request_count);

    printf("[SOCIAL] Friend requests list sent (%d requests)\n", request_count);
}
