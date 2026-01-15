#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "handlers/friend_handler.h"
#include "handlers/auth_guard.h"
#include "handlers/session_manager.h"
#include "handlers/session_context.h"
#include "db/repo/friend_repo.h"
#include "db/repo/account_repo.h"
#include "db/repo/profile_repo.h"
#include "protocol/protocol.h"
#include "protocol/opcode.h"

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static void send_response(
    int client_fd,
    MessageHeader *req,
    uint16_t response_code,
    cJSON *payload
) {
    char *json_str = cJSON_PrintUnformatted(payload);
    if (!json_str) {
        printf("[FRIEND] Failed to serialize JSON\n");
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

// ============================================================================
// COMMAND HANDLERS
// ============================================================================

void handle_friend_add(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[FRIEND] Processing add friend request\n");

    // Authenticate user
    if (!require_auth(client_fd, header)) {
        return;
    }
    int32_t user_id = get_client_account(client_fd);

    // Parse JSON payload
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    // Get friend identifier (email or id)
    cJSON *email_json = cJSON_GetObjectItem(json, "friend_email");
    cJSON *id_json = cJSON_GetObjectItem(json, "friend_id");

    int32_t friend_id = 0;

    if (email_json && cJSON_IsString(email_json)) {
        // Search by email
        const char *email = email_json->valuestring;
        account_t *friend_account = NULL;
        db_error_t err = account_find_by_email(email, &friend_account);
        
        if (err != DB_SUCCESS || !friend_account) {
            cJSON_Delete(json);
            send_error(client_fd, header, ERR_FRIEND_NOT_FOUND, "User not found");
            return;
        }

        friend_id = friend_account->id;
        account_free(friend_account);
    } else if (id_json && cJSON_IsNumber(id_json)) {
        // Direct ID
        friend_id = id_json->valueint;
    } else {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing friend_email or friend_id");
        return;
    }

    // Validate friend_id
    if (friend_id <= 0) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_FRIEND_NOT_FOUND, "Invalid friend ID");
        return;
    }

    // Cannot add self
    if (friend_id == user_id) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Cannot add yourself as friend");
        return;
    }

    // Check if already friends
    bool are_friends = false;
    friend_check_relationship(user_id, friend_id, &are_friends);
    if (are_friends) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_CONFLICT, "Already friends");
        return;
    }

    // Create friend request
    friend_request_t *request = NULL;
    db_error_t err = friend_request_create(user_id, friend_id, &request);

    if (err == DB_ERROR_INVALID_PARAM) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Cannot add yourself as friend");
        return;
    }

    if (err == DB_ERR_CONFLICT) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_CONFLICT, "Friend request already sent or already friends");
        return;
    }

    if (err != DB_SUCCESS || !request) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to create friend request");
        return;
    }

    // Build success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "request_id", request->id);
    cJSON_AddStringToObject(response, "message", "Friend request sent");

    send_response(client_fd, header, RES_FRIEND_ADDED, response);

    printf("[FRIEND] Friend request sent: user=%d -> friend=%d, request_id=%d\n", 
           user_id, friend_id, request->id);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
    friend_request_free(request);
}

void handle_friend_accept(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[FRIEND] Processing accept friend request\n");

    // Authenticate user
    if (!require_auth(client_fd, header)) {
        return;
    }
    int32_t user_id = get_client_account(client_fd);

    // Parse JSON payload
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *request_id_json = cJSON_GetObjectItem(json, "request_id");
    if (!request_id_json || !cJSON_IsNumber(request_id_json)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing request_id");
        return;
    }

    int32_t request_id = request_id_json->valueint;

    // Accept the request
    friend_request_t *request = NULL;
    db_error_t err = friend_request_accept(request_id, &request);

    if (err != DB_SUCCESS || !request) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_NOT_FOUND, "Friend request not found");
        return;
    }

    // Verify this user is the receiver
    if (request->receiver_id != user_id) {
        cJSON_Delete(json);
        friend_request_free(request);
        send_error(client_fd, header, ERR_FORBIDDEN, "Not authorized to accept this request");
        return;
    }

    // Build success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "friend_id", request->sender_id);
    cJSON_AddStringToObject(response, "message", "Friend request accepted");

    send_response(client_fd, header, RES_FRIEND_REQUEST_ACCEPTED, response);

    printf("[FRIEND] Friend request accepted: request_id=%d, sender=%d, receiver=%d\n", 
           request_id, request->sender_id, request->receiver_id);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
    friend_request_free(request);
}

void handle_friend_reject(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[FRIEND] Processing reject friend request\n");

    // Authenticate user
    if (!require_auth(client_fd, header)) {
        return;
    }
    int32_t user_id = get_client_account(client_fd);

    // Parse JSON payload
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *request_id_json = cJSON_GetObjectItem(json, "request_id");
    if (!request_id_json || !cJSON_IsNumber(request_id_json)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing request_id");
        return;
    }

    int32_t request_id = request_id_json->valueint;

    // Reject the request
    friend_request_t *request = NULL;
    db_error_t err = friend_request_reject(request_id, &request);

    if (err != DB_SUCCESS || !request) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_NOT_FOUND, "Friend request not found");
        return;
    }

    // Verify this user is the receiver
    if (request->receiver_id != user_id) {
        cJSON_Delete(json);
        friend_request_free(request);
        send_error(client_fd, header, ERR_FORBIDDEN, "Not authorized to reject this request");
        return;
    }

    // Build success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Friend request rejected");

    send_response(client_fd, header, RES_FRIEND_REQUEST_REJECTED, response);

    printf("[FRIEND] Friend request rejected: request_id=%d, sender=%d, receiver=%d\n", 
           request_id, request->sender_id, request->receiver_id);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
    friend_request_free(request);
}

void handle_friend_remove(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[FRIEND] Processing remove friend\n");

    // Authenticate user
    if (!require_auth(client_fd, header)) {
        return;
    }
    int32_t user_id = get_client_account(client_fd);

    // Parse JSON payload
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

    // Remove friendship
    db_error_t err = friend_remove(user_id, friend_id);

    if (err != DB_SUCCESS) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_FRIEND_NOT_FOUND, "Friend not found or already removed");
        return;
    }

    // Build success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Friend removed");

    send_response(client_fd, header, RES_FRIEND_REMOVED, response);

    printf("[FRIEND] Friendship removed: user=%d, friend=%d\n", user_id, friend_id);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
}

void handle_friend_list(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[FRIEND] Processing get friend list\n");
    (void)payload;

    // Authenticate user
    if (!require_auth(client_fd, header)) {
        return;
    }
    int32_t user_id = get_client_account(client_fd);

    // Get friend IDs
    int32_t *friend_ids = NULL;
    int32_t count = 0;
    db_error_t err = friend_list_get_ids(user_id, &friend_ids, &count);

    if (err != DB_SUCCESS) {
        send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to fetch friends");
        return;
    }

    // Build friends array with details
    cJSON *friends_array = cJSON_CreateArray();

    for (int32_t i = 0; i < count; i++) {
        int32_t friend_id = friend_ids[i];

        // Get friend's profile
        profile_t *profile = NULL;
        db_error_t profile_err = profile_find_by_account(friend_id, &profile);

        if (profile_err != DB_SUCCESS || !profile) {
            continue;
        }

        // Get friend's account for email
        account_t *account = NULL;
        account_find_by_id(friend_id, &account);

        // Check if friend is online
        UserSession *friend_session = session_get_by_account(friend_id);
        bool is_online = (friend_session != NULL && friend_session->state != SESSION_UNAUTHENTICATED);
        bool in_game = (friend_session != NULL && friend_session->state == SESSION_PLAYING);

        cJSON *friend_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(friend_obj, "id", friend_id);
        if (profile->name) cJSON_AddStringToObject(friend_obj, "name", profile->name);
        if (account && account->email) cJSON_AddStringToObject(friend_obj, "email", account->email);
        if (profile->avatar) cJSON_AddStringToObject(friend_obj, "avatar", profile->avatar);
        cJSON_AddNumberToObject(friend_obj, "points", profile->points);
        cJSON_AddNumberToObject(friend_obj, "wins", profile->wins);
        cJSON_AddNumberToObject(friend_obj, "matches", profile->matches);
        cJSON_AddBoolToObject(friend_obj, "online", is_online);
        cJSON_AddBoolToObject(friend_obj, "in_game", in_game);

        cJSON_AddItemToArray(friends_array, friend_obj);

        profile_free(profile);
        if (account) account_free(account);
    }

    // Build response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddItemToObject(response, "friends", friends_array);
    cJSON_AddNumberToObject(response, "count", cJSON_GetArraySize(friends_array));

    send_response(client_fd, header, RES_FRIEND_LIST, response);

    printf("[FRIEND] Friend list sent: user=%d, count=%d\n", user_id, cJSON_GetArraySize(friends_array));

    // Cleanup
    cJSON_Delete(response);
    if (friend_ids) free(friend_ids);
}

void handle_friend_requests(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[FRIEND] Processing get friend requests\n");
    (void)payload;

    // Authenticate user
    if (!require_auth(client_fd, header)) {
        return;
    }
    int32_t user_id = get_client_account(client_fd);

    // Get pending friend requests
    friend_request_t **requests = NULL;
    int32_t count = 0;
    db_error_t err = friend_request_list(user_id, &requests, &count);

    if (err != DB_SUCCESS) {
        send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to fetch friend requests");
        return;
    }

    // Build requests array with sender details
    cJSON *requests_array = cJSON_CreateArray();

    for (int32_t i = 0; i < count; i++) {
        friend_request_t *req = requests[i];
        if (!req) continue;

        // Get sender's profile
        profile_t *profile = NULL;
        db_error_t profile_err = profile_find_by_account(req->sender_id, &profile);

        // Get sender's account for email
        account_t *account = NULL;
        account_find_by_id(req->sender_id, &account);

        cJSON *request_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(request_obj, "id", req->id);
        cJSON_AddNumberToObject(request_obj, "sender_id", req->sender_id);

        if (profile && profile->name) {
            cJSON_AddStringToObject(request_obj, "sender_name", profile->name);
        } else {
            cJSON_AddStringToObject(request_obj, "sender_name", "Unknown");
        }

        if (account && account->email) {
            cJSON_AddStringToObject(request_obj, "sender_email", account->email);
        }

        if (profile && profile->avatar) {
            cJSON_AddStringToObject(request_obj, "sender_avatar", profile->avatar);
        }

        if (req->status) {
            cJSON_AddStringToObject(request_obj, "status", req->status);
        }

        cJSON_AddItemToArray(requests_array, request_obj);

        if (profile) profile_free(profile);
        if (account) account_free(account);
    }

    // Build response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddItemToObject(response, "requests", requests_array);
    cJSON_AddNumberToObject(response, "count", cJSON_GetArraySize(requests_array));

    send_response(client_fd, header, RES_FRIEND_REQUESTS, response);

    printf("[FRIEND] Friend requests sent: user=%d, count=%d\n", user_id, cJSON_GetArraySize(requests_array));

    // Cleanup
    cJSON_Delete(response);
    friend_request_free_array(requests, count);
}

void handle_search_user(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[FRIEND] Processing search user\n");

    // Authenticate user
    if (!require_auth(client_fd, header)) {
        return;
    }
    int32_t user_id = get_client_account(client_fd);

    // Parse JSON payload
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *query_json = cJSON_GetObjectItem(json, "query");
    if (!query_json || !cJSON_IsString(query_json)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing query");
        return;
    }

    const char *search_query = query_json->valuestring;

    // Parse query as account ID (numeric)
    char *endptr;
    int32_t search_account_id = (int32_t)strtol(search_query, &endptr, 10);
    
    // If not a valid number, treat as error
    if (*endptr != '\0' || search_account_id <= 0) {
        cJSON_Delete(json);
        cJSON *response = cJSON_CreateObject();
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddItemToObject(response, "users", cJSON_CreateArray());
        cJSON_AddNumberToObject(response, "count", 0);
        send_response(client_fd, header, RES_SUCCESS, response);
        cJSON_Delete(response);
        printf("[FRIEND] Invalid search query (not a number): query='%s'\n", search_query);
        return;
    }

    // Search for user by account ID
    account_t *account = NULL;
    db_error_t err = account_find_by_id(search_account_id, &account);

    cJSON *users_array = cJSON_CreateArray();

    if (err == DB_SUCCESS && account) {
        // Don't show self in search results
        if (account->id == user_id) {
            cJSON *response = cJSON_CreateObject();
            cJSON_AddBoolToObject(response, "success", true);
            cJSON_AddItemToObject(response, "users", cJSON_CreateArray());
            cJSON_AddNumberToObject(response, "count", 0);
            send_response(client_fd, header, RES_SUCCESS, response);
            cJSON_Delete(response);
            cJSON_Delete(json);
            account_free(account);
            printf("[FRIEND] Search result is self, returning empty results\n");
            return;
        }

        // Found user by exact email match
        profile_t *profile = NULL;
        profile_find_by_account(account->id, &profile);

        // Check if already friends
        bool is_friend = false;
        friend_check_relationship(user_id, account->id, &is_friend);

        // TODO: Check if friend request pending

        cJSON *user_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(user_obj, "id", account->id);
        if (profile && profile->name) {
            cJSON_AddStringToObject(user_obj, "name", profile->name);
        } else {
            cJSON_AddStringToObject(user_obj, "name", "Unknown");
        }
        cJSON_AddStringToObject(user_obj, "email", account->email);
        if (profile && profile->avatar) {
            cJSON_AddStringToObject(user_obj, "avatar", profile->avatar);
        }
        cJSON_AddBoolToObject(user_obj, "is_friend", is_friend);
        cJSON_AddBoolToObject(user_obj, "request_pending", false); // TODO

        cJSON_AddItemToArray(users_array, user_obj);

        if (profile) profile_free(profile);
        account_free(account);
    }

    // Build response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddItemToObject(response, "users", users_array);
    cJSON_AddNumberToObject(response, "count", cJSON_GetArraySize(users_array));

    send_response(client_fd, header, RES_SUCCESS, response);

    printf("[FRIEND] User search completed: query='%s', results=%d\n", 
           search_query, cJSON_GetArraySize(users_array));

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
}

// ============================================================================
// MAIN DISPATCHER
// ============================================================================

void handle_friend(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[FRIEND] Handling friend command: 0x%04X\n", header->command);

    switch (header->command) {
        case CMD_FRIEND_ADD:
            handle_friend_add(client_fd, header, payload);
            break;

        case CMD_FRIEND_ACCEPT:
            handle_friend_accept(client_fd, header, payload);
            break;

        case CMD_FRIEND_REJECT:
            handle_friend_reject(client_fd, header, payload);
            break;

        case CMD_FRIEND_REMOVE:
            handle_friend_remove(client_fd, header, payload);
            break;

        case CMD_FRIEND_LIST:
            handle_friend_list(client_fd, header, payload);
            break;

        case CMD_FRIEND_REQUESTS:
            handle_friend_requests(client_fd, header, payload);
            break;

        case CMD_SEARCH_USER:
            handle_search_user(client_fd, header, payload);
            break;

        default:
            printf("[FRIEND] Unknown friend command: 0x%04X\n", header->command);
            send_error(client_fd, header, ERR_BAD_REQUEST, "Unknown friend command");
            break;
    }
}
