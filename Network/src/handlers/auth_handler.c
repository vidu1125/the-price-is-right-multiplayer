#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "handlers/auth_handler.h"
#include "db/repo/account_repo.h"
#include "db/repo/session_repo.h"
#include "db/repo/profile_repo.h"
#include "handlers/session_context.h"
#include "transport/room_manager.h"
#include "handlers/session_manager.h"
#include "protocol/protocol.h"
#include "protocol/opcode.h"
#include "utils/crypto.h"

// Helper function to send response
static void send_response(
    int client_fd,
    MessageHeader *req,
    uint16_t response_code,
    cJSON *payload
) {
    char *json_str = cJSON_PrintUnformatted(payload);
    if (!json_str) {
        printf("[ERROR] Failed to serialize JSON\n");
        return;
    }

    forward_response(client_fd, req, response_code, json_str, strlen(json_str));
    free(json_str);
}

// Helper function to send error
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

static void add_profile_to_response(cJSON *response, profile_t *profile, account_t *account) {
    cJSON *profile_json = cJSON_CreateObject();
    if (profile) {
        cJSON_AddNumberToObject(profile_json, "id", profile->id);
        cJSON_AddNumberToObject(profile_json, "account_id", profile->account_id);
        if (profile->name) cJSON_AddStringToObject(profile_json, "name", profile->name);
        if (profile->avatar) cJSON_AddStringToObject(profile_json, "avatar", profile->avatar);
        if (profile->bio) cJSON_AddStringToObject(profile_json, "bio", profile->bio);
        cJSON_AddNumberToObject(profile_json, "matches", profile->matches);
        cJSON_AddNumberToObject(profile_json, "wins", profile->wins);
        cJSON_AddNumberToObject(profile_json, "points", profile->points);
        if (profile->badges) cJSON_AddStringToObject(profile_json, "badges", profile->badges);
    }

    if (account) {
        cJSON_AddStringToObject(profile_json, "email", account->email);
        cJSON_AddStringToObject(profile_json, "role", account->role);
    }

    cJSON_AddItemToObject(response, "profile", profile_json);
}

void handle_login(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[AUTH] ========== LOGIN REQUEST ==========\n");
    (void)header;

    // Parse JSON payload
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        printf("[AUTH] Failed to parse JSON payload\n");
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *email_json = cJSON_GetObjectItem(json, "email");
    cJSON *password_json = cJSON_GetObjectItem(json, "password");

    if (!email_json || !cJSON_IsString(email_json) ||
        !password_json || !cJSON_IsString(password_json)) {
        printf("[AUTH] Missing email or password in JSON\n");
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing email or password");
        return;
    }

    const char *email = email_json->valuestring;
    const char *password = password_json->valuestring;

    printf("[AUTH] Login attempt: email=%s\n", email);

    // Validate email format
    if (!crypto_validate_email(email)) {
        printf("[AUTH] Invalid email format: %s\n", email);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid email format");
        return;
    }

    // Validate Gmail domain
    const char *at_sign = strchr(email, '@');
    if (!at_sign || strcasecmp(at_sign, "@gmail.com") != 0) {
        printf("[AUTH] Email is not Gmail: %s\n", email);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Email must be a valid Gmail address (@gmail.com)");
        return;
    }

    // Find account by email
    account_t *account = NULL;
    db_error_t err = account_find_by_email(email, &account);
    
    if (err != DB_SUCCESS || !account) {
        printf("[AUTH] Account lookup failed: err=%d, account=%p\n", err, account);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_INVALID_USERNAME, "Invalid email or password");
        return;
    }

    printf("[AUTH] Account found: id=%d\n", account->id);

    // Verify password
    printf("[AUTH] Verifying password...\n");
    if (!crypto_verify_password(password, account->password)) {
        printf("[AUTH] Password verification failed\n");
        account_free(account);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_INVALID_USERNAME, "Invalid email or password");
        return;
    }

    printf("[AUTH] Password verified successfully\n");

    // Create or update session
    session_t *session = NULL;
    err = session_find_by_account(account->id, &session);
    
    if (err == DB_SUCCESS && session) {
        // Existing session - reconnect
        session_reconnect(session->session_id);
    } else {
        // Create new session
        session_free(session);
        err = session_create(account->id, &session);
        if (err != DB_SUCCESS || !session) {
            account_free(account);
            cJSON_Delete(json);
            send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to create session");
            return;
        }
    }

    // Fetch profile (create default if missing)
    profile_t *profile = NULL;
    db_error_t profile_err = profile_find_by_account(account->id, &profile);
    if (profile_err == DB_ERROR_NOT_FOUND) {
        profile_create(account->id, NULL, NULL, NULL, &profile);
    }

    // TODO: Fetch profile data
    // For now, send basic response

    // Enforce session exclusivity and bind mapping per session rule
    UserSession *bound = session_bind_after_login(client_fd, account->id, session->session_id, header);
    if (!bound) {
        // Blocked by session rule (playing and connected); close new login
        cJSON_Delete(json);
        account_free(account);
        session_free(session);
        profile_free(profile);
        return;
    }

    // Bind legacy mapping for handlers that rely on it
    set_client_session(client_fd, session->session_id, account->id);

    // Build success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "account_id", account->id);
    cJSON_AddStringToObject(response, "session_id", session->session_id);
    add_profile_to_response(response, profile, account);

    send_response(client_fd, header, RES_LOGIN_OK, response);

    printf("[AUTH] Login successful: account_id=%d, session_id=%s\n", 
           account->id, session->session_id);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
    account_free(account);
    session_free(session);
    profile_free(profile);
}

void handle_register(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[AUTH] Processing registration request\n");
    (void)header;

    const size_t MIN_PASSWORD = 6;
    const size_t MAX_PASSWORD = 72; // bcrypt practical limit

    // Parse JSON payload
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *email_json = cJSON_GetObjectItem(json, "email");
    cJSON *password_json = cJSON_GetObjectItem(json, "password");
    cJSON *confirm_json = cJSON_GetObjectItem(json, "confirm");
    if (!confirm_json) {
        confirm_json = cJSON_GetObjectItem(json, "confirm_password");
    }
    cJSON *name_json = cJSON_GetObjectItem(json, "name");

    if (!email_json || !cJSON_IsString(email_json) ||
        !password_json || !cJSON_IsString(password_json)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing email or password");
        return;
    }

    const char *email = email_json->valuestring;
    const char *password = password_json->valuestring;
    const char *confirm = (confirm_json && cJSON_IsString(confirm_json)) ? confirm_json->valuestring : NULL;

    // Validate email format
    if (!crypto_validate_email(email)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid email format");
        return;
    }

    // Validate Gmail domain
    const char *at_sign = strchr(email, '@');
    if (!at_sign || strcasecmp(at_sign, "@gmail.com") != 0) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Email must be a valid Gmail address (@gmail.com)");
        return;
    }

    // Validate password strength & match
    size_t pwd_len = strlen(password);
    if (pwd_len < MIN_PASSWORD) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Password must be at least 6 characters");
        return;
    }
    if (pwd_len > MAX_PASSWORD) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Password is too long");
        return;
    }
    if (confirm && strcmp(password, confirm) != 0) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Passwords do not match");
        return;
    }

    // Hash password
    char password_hash[61];
    if (!crypto_hash_password(password, password_hash)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to hash password");
        return;
    }

    // Create account
    account_t *account = NULL;
    db_error_t err = account_create(email, password_hash, "user", &account);
    
    if (err == DB_ERR_CONFLICT) {
        printf("[AUTH] Email conflict: %s\n", email);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_INVALID_USERNAME, "Email already registered");
        return;
    }

    if (err != DB_SUCCESS) {
        printf("[AUTH] Failed to create account: err=%d\n", err);
        cJSON_Delete(json);
        
        switch(err) {
            case DB_ERROR_INVALID_PARAM:
                send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid account parameters");
                break;
            case DB_ERR_HTTP:
            case DB_ERR_TIMEOUT:
                send_error(client_fd, header, ERR_SERVER_ERROR, "Database connection failed");
                break;
            case DB_ERROR_PARSE:
                send_error(client_fd, header, ERR_SERVER_ERROR, "Invalid database response");
                break;
            default:
                printf("[AUTH] Account creation error code: %d\n", err);
                send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to create account");
                break;
        }
        return;
    }
    
    if (!account) {
        printf("[AUTH] Account creation returned NULL\n");
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_SERVER_ERROR, "Account creation failed");
        return;
    }

    // Create profile for the account (name optional)
    const char *name_val = NULL;
    if (name_json && cJSON_IsString(name_json) && strlen(name_json->valuestring) > 0) {
        name_val = name_json->valuestring;
    }

    profile_t *profile = NULL;
    err = profile_create(account->id, name_val, NULL, NULL, &profile);
    if (err != DB_SUCCESS) {
        cJSON_Delete(json);
        account_free(account);
        send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to create profile");
        return;
    }

    // Create session immediately so user is logged in after register
    session_t *session = NULL;
    err = session_create(account->id, &session);
    if (err != DB_SUCCESS || !session) {
        cJSON_Delete(json);
        account_free(account);
        profile_free(profile);
        send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to create session");
        return;
    }

    // Build success response with session + profile
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "account_id", account->id);
    cJSON_AddStringToObject(response, "session_id", session->session_id);
    cJSON_AddStringToObject(response, "message", "Registration successful");
    add_profile_to_response(response, profile, account);

    send_response(client_fd, header, RES_SUCCESS, response);

    // Bind session to this connection for downstream commands
    set_client_session(client_fd, session->session_id, account->id);

    printf("[AUTH] Registration successful: account_id=%d, email=%s\n", 
           account->id, email);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
    account_free(account);
    session_free(session);
    profile_free(profile);
}

void handle_logout(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[AUTH] Processing logout request\n");
    (void)header;

    // Parse JSON payload
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *session_id_json = cJSON_GetObjectItem(json, "session_id");

    if (!session_id_json || !cJSON_IsString(session_id_json)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing session_id");
        return;
    }

    const char *session_id = session_id_json->valuestring;

    // Find session
    session_t *session = NULL;
    db_error_t err = session_find_by_id(session_id, &session);
    
    if (err != DB_SUCCESS || !session) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Session not found");
        return;
    }

    // TODO: Remove player from any active rooms
    room_remove_member_all(client_fd);

    // Mark session disconnected in DB and keep record
    printf("[AUTH] Marking session disconnected: %s\n", session_id);
    db_error_t update_err = session_update_connected(session_id, false);
    if (update_err != DB_SUCCESS) {
        printf("[AUTH] Failed to update session connected status: err=%d\n", update_err);
    } else {
        printf("[AUTH] Session marked disconnected successfully\n");
    }

    // Clear binding and session state
    clear_client_session(client_fd);
    UserSession *us = session_get_by_socket(client_fd);
    if (us) {
        session_destroy(us);
    }

    // Build success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Logged out successfully");

    send_response(client_fd, header, RES_SUCCESS, response);

    printf("[AUTH] Logout successful: session_id=%s\n", session_id);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
    session_free(session);
}

void handle_reconnect(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[AUTH] Processing reconnect request\n");
    (void)header;

    // Parse JSON payload
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *session_id_json = cJSON_GetObjectItem(json, "session_id");

    if (!session_id_json || !cJSON_IsString(session_id_json)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing session_id");
        return;
    }

    const char *session_id = session_id_json->valuestring;

    // Find session
    session_t *session = NULL;
    db_error_t err = session_find_by_id(session_id, &session);
    
    if (err != DB_SUCCESS || !session) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Invalid or expired session");
        return;
    }

    // Find account
    account_t *account = NULL;
    err = account_find_by_id(session->account_id, &account);
    
    if (err != DB_SUCCESS || !account) {
        session_free(session);
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_NOT_LOGGED_IN, "Account not found");
        return;
    }

    // Update session as connected
    session_reconnect(session_id);

    // Bind per session rule (reconnect from DISCONNECTED)
    UserSession *bound = session_bind_after_login(client_fd, account->id, session_id, header);
    if (!bound) {
        // if blocked, return
        cJSON_Delete(json);
        account_free(account);
        session_free(session);
        return;
    }

    // Legacy mapping for other handlers
    set_client_session(client_fd, session_id, account->id);

    // Fetch profile (best effort)
    profile_t *profile = NULL;
    db_error_t profile_err = profile_find_by_account(account->id, &profile);
    if (profile_err == DB_ERROR_NOT_FOUND) {
        profile_create(account->id, NULL, NULL, NULL, &profile);
    }

    // TODO: Restore room state if player was in a room

    // Build success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "account_id", account->id);
    cJSON_AddStringToObject(response, "session_id", session->session_id);
    add_profile_to_response(response, profile, account);

    send_response(client_fd, header, RES_LOGIN_OK, response);

    printf("[AUTH] Reconnect successful: account_id=%d, session_id=%s\n", 
           account->id, session->session_id);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
    account_free(account);
    session_free(session);
    profile_free(profile);
}
