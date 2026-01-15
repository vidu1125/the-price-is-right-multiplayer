#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "handlers/password_handler.h"
#include "handlers/session_context.h"
#include "db/repo/account_repo.h"
#include "utils/crypto.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"

// Helper to send JSON responses
static void send_response(
    int client_fd,
    MessageHeader *req,
    uint16_t response_code,
    cJSON *payload
) {
    char *json_str = cJSON_PrintUnformatted(payload);
    if (!json_str) {
        printf("[PASSWORD_HANDLER] Failed to serialize response\n");
        return;
    }
    forward_response(client_fd, req, response_code, json_str, strlen(json_str));
    free(json_str);
}

// Helper to send error
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

void handle_change_password(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    printf("[PASSWORD_HANDLER] Change password request received\n");
    
    // Check Payload Length
    if (header->length < sizeof(ChangePasswordRequest)) {
        send_error(client_fd, header, RES_PASSWORD_CHANGED, "Invalid payload size");
        return;
    }

    const ChangePasswordRequest *req = (const ChangePasswordRequest *)payload;

    // Get account_id from session
    int32_t account_id = get_client_account(client_fd);
    if (account_id <= 0) {
        send_error(client_fd, header, RES_PASSWORD_CHANGED, "Not logged in");
        return;
    }

    // Ensure null termination for safety (though struct is fixed size, strings might not be null-terminated if full)
    char old_password[65] = {0};
    char new_password[65] = {0};
    memcpy(old_password, req->old_password, 64);
    memcpy(new_password, req->new_password, 64);
    // Ensure last char is null just in case
    old_password[64] = '\0';
    new_password[64] = '\0';

    // Validate new password strength (basic)
    if (strlen(new_password) < 6) {
        send_error(client_fd, header, RES_PASSWORD_CHANGED, "New password too short");
        return;
    }

    // Fetch account to verify old password
    account_t *account = NULL;
    db_error_t err = account_find_by_id(account_id, &account);
    if (err != DB_SUCCESS || !account) {
        send_error(client_fd, header, RES_PASSWORD_CHANGED, "Account not found");
        return;
    }

    // Verify old password
    if (!crypto_verify_password(old_password, account->password)) {
        account_free(account);
        send_error(client_fd, header, RES_PASSWORD_CHANGED, "Incorrect old password");
        return;
    }

    // Hash new password
    char new_hash[64]; // crypto_hash_password uses a buffer of decent size
    if (!crypto_hash_password(new_password, new_hash)) {
        account_free(account);
        send_error(client_fd, header, RES_PASSWORD_CHANGED, "Password hashing failed");
        return;
    }

    // Update password in DB
    err = account_update_password(account_id, new_hash);
    
    if (err != DB_SUCCESS) {
        account_free(account);
        send_error(client_fd, header, RES_PASSWORD_CHANGED, "Failed to update password");
        return;
    }

    // Success response
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON_AddStringToObject(resp, "message", "Password changed successfully");
    
    send_response(client_fd, header, RES_PASSWORD_CHANGED, resp);

    printf("[PASSWORD_HANDLER] Password changed successfully for account %d\n", account_id);

    // Cleanup
    cJSON_Delete(resp);
    account_free(account);
}
