#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "handlers/auth_handler.h"
#include "db/repo/account_repo.h"
#include "db/repo/session_repo.h"
#include "db/repo/profile_repo.h"
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

    // TODO: Fetch profile data
    // For now, send basic response

    // Build success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "account_id", account->id);
    cJSON_AddStringToObject(response, "session_id", session->session_id);
    
    cJSON *profile = cJSON_CreateObject();
    cJSON_AddStringToObject(profile, "email", account->email);
    cJSON_AddStringToObject(profile, "role", account->role);
    cJSON_AddItemToObject(response, "profile", profile);

    send_response(client_fd, header, RES_LOGIN_OK, response);

    printf("[AUTH] Login successful: account_id=%d, session_id=%s\n", 
           account->id, session->session_id);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
    account_free(account);
    session_free(session);
}

void handle_register(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[AUTH] Processing registration request\n");
    (void)header;

    // Parse JSON payload
    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *email_json = cJSON_GetObjectItem(json, "email");
    cJSON *password_json = cJSON_GetObjectItem(json, "password");
    cJSON *name_json = cJSON_GetObjectItem(json, "name");
    (void)name_json;

    if (!email_json || !cJSON_IsString(email_json) ||
        !password_json || !cJSON_IsString(password_json)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Missing email or password");
        return;
    }

    const char *email = email_json->valuestring;
    const char *password = password_json->valuestring;
    (void)name_json; // Use name_json if needed for profile creation
    // const char *name = name_json && cJSON_IsString(name_json) ? 
    //                    name_json->valuestring : "Player";

    // Validate email format
    if (!crypto_validate_email(email)) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Invalid email format");
        return;
    }

    // Validate password length
    if (strlen(password) < 6) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_BAD_REQUEST, "Password must be at least 6 characters");
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
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_INVALID_USERNAME, "Email already registered");
        return;
    }

    if (err != DB_SUCCESS || !account) {
        cJSON_Delete(json);
        send_error(client_fd, header, ERR_SERVER_ERROR, "Failed to create account");
        return;
    }

    // TODO: Create profile for the account

    // Build success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "account_id", account->id);
    cJSON_AddStringToObject(response, "message", "Registration successful");

    send_response(client_fd, header, RES_SUCCESS, response);

    printf("[AUTH] Registration successful: account_id=%d, email=%s\n", 
           account->id, email);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
    account_free(account);
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

    // Delete session
    session_delete(session_id);

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

    // TODO: Restore room state if player was in a room

    // Build success response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "account_id", account->id);
    cJSON_AddStringToObject(response, "session_id", session->session_id);
    
    cJSON *profile = cJSON_CreateObject();
    cJSON_AddStringToObject(profile, "email", account->email);
    cJSON_AddStringToObject(profile, "role", account->role);
    cJSON_AddItemToObject(response, "profile", profile);

    send_response(client_fd, header, RES_LOGIN_OK, response);

    printf("[AUTH] Reconnect successful: account_id=%d, session_id=%s\n", 
           account->id, session->session_id);

    // Cleanup
    cJSON_Delete(response);
    cJSON_Delete(json);
    account_free(account);
    session_free(session);
}
