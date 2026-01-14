#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <stdbool.h>

#include "db/repo/account_repo.h"
#include "db/core/db_client.h"

// Helper function to parse account from JSON
static account_t* parse_account_from_json(cJSON *json) {
    if (!json) return NULL;

    account_t *account = calloc(1, sizeof(account_t));
    if (!account) return NULL;

    cJSON *id = cJSON_GetObjectItem(json, "id");
    cJSON *email = cJSON_GetObjectItem(json, "email");
    cJSON *password = cJSON_GetObjectItem(json, "password");
    cJSON *role = cJSON_GetObjectItem(json, "role");
    (void)cJSON_GetObjectItem(json, "created_at");
    (void)cJSON_GetObjectItem(json, "updated_at");

    if (id && cJSON_IsNumber(id)) {
        account->id = id->valueint;
    }

    if (email && cJSON_IsString(email)) {
        account->email = strdup(email->valuestring);
    }

    if (password && cJSON_IsString(password)) {
        account->password = strdup(password->valuestring);
    }

    if (role && cJSON_IsString(role)) {
        account->role = strdup(role->valuestring);
    } else {
        account->role = strdup("user");
    }

    // TODO: Parse timestamps if needed
    account->created_at = 0;
    account->updated_at = 0;

    return account;
}

// Minimal URL encode for email (handles @ and space)
static void encode_email(const char *email, char *out, size_t out_size) {
    size_t o = 0;
    for (size_t i = 0; email && email[i] && o + 4 < out_size; i++) {
        char c = email[i];
        if (c == '@') {
            if (o + 3 >= out_size) break;
            out[o++] = '%'; out[o++] = '4'; out[o++] = '0';
        } else if (c == ' ') {
            if (o + 3 >= out_size) break;
            out[o++] = '%'; out[o++] = '2'; out[o++] = '0';
        } else {
            out[o++] = c;
        }
    }
    out[o] = '\0';
}

db_error_t account_create(
    const char *email,
    const char *password_hash,
    const char *role,
    account_t **out_account
) {
    if (!email || !password_hash || !out_account) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Create JSON payload
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "email", email);
    cJSON_AddStringToObject(payload, "password", password_hash);
    cJSON_AddStringToObject(payload, "role", role ? role : "user");

    cJSON *response = NULL;
    db_error_t err = db_post("accounts", payload, &response);
    cJSON_Delete(payload);

    if (err != DB_SUCCESS) {
        if (response) cJSON_Delete(response);
        return err;
    }

    // Parse response - should be an array with one element
    if (cJSON_IsArray(response) && cJSON_GetArraySize(response) > 0) {
        cJSON *item = cJSON_GetArrayItem(response, 0);
        *out_account = parse_account_from_json(item);
        cJSON_Delete(response);
        return *out_account ? DB_SUCCESS : DB_ERROR_PARSE;
    }

    cJSON_Delete(response);
    return DB_ERROR_PARSE;
}

db_error_t account_find_by_email(
    const char *email,
    account_t **out_account
) {
    if (!email || !out_account) {
        printf("[AUTH] Invalid email or out_account\n");
        return DB_ERROR_INVALID_PARAM;
    }

    printf("[AUTH] Finding account by email: %s\n", email);

    // Build SQL query instead of REST query
    char query[512];
    snprintf(query, sizeof(query), "SELECT * FROM accounts WHERE email = '%s' LIMIT 1", email);

    cJSON *response = NULL;
    db_error_t err = db_get("accounts", query, &response);

    printf("[AUTH] Query result: %d\n", err);
    
    if (err != DB_SUCCESS) {
        printf("[AUTH] DB error: %d\n", err);
        if (response) cJSON_Delete(response);
        return err;
    }

    // Parse response - should be an array
    if (cJSON_IsArray(response)) {
        int count = cJSON_GetArraySize(response);
        printf("[AUTH] Found %d accounts\n", count);
        
        if (count == 0) {
            printf("[AUTH] No account found for email: %s\n", email);
            cJSON_Delete(response);
            return DB_ERROR_NOT_FOUND;
        }

        cJSON *item = cJSON_GetArrayItem(response, 0);
        *out_account = parse_account_from_json(item);
        
        if (*out_account) {
            printf("[AUTH] Account found: id=%d, email=%s\n", (*out_account)->id, (*out_account)->email);
        }
        
        cJSON_Delete(response);
        return *out_account ? DB_SUCCESS : DB_ERROR_PARSE;
    }

    printf("[AUTH] Response is not an array\n");
    cJSON_Delete(response);
    return DB_ERROR_PARSE;
}

db_error_t account_find_by_id(
    int32_t account_id,
    account_t **out_account
) {
    if (account_id <= 0 || !out_account) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Build SQL query
    char query[256];
    snprintf(query, sizeof(query), "SELECT * FROM accounts WHERE id = %d LIMIT 1", account_id);

    cJSON *response = NULL;
    db_error_t err = db_get("accounts", query, &response);

    if (err != DB_SUCCESS) {
        if (response) cJSON_Delete(response);
        return err;
    }

    // Parse response
    if (cJSON_IsArray(response)) {
        int count = cJSON_GetArraySize(response);
        if (count == 0) {
            cJSON_Delete(response);
            return DB_ERROR_NOT_FOUND;
        }

        cJSON *item = cJSON_GetArrayItem(response, 0);
        *out_account = parse_account_from_json(item);
        cJSON_Delete(response);
        return *out_account ? DB_SUCCESS : DB_ERROR_PARSE;
    }

    cJSON_Delete(response);
    return DB_ERROR_PARSE;
}

db_error_t account_update_password(
    int32_t account_id,
    const char *new_password_hash
) {
    if (account_id <= 0 || !new_password_hash) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Build SQL filter
    char filter[128];
    snprintf(filter, sizeof(filter), "id = %d", account_id);

    // Create payload
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "password", new_password_hash);

    cJSON *response = NULL;
    db_error_t err = db_patch("accounts", filter, payload, &response);
    
    cJSON_Delete(payload);
    if (response) cJSON_Delete(response);
    
    return err;
}

db_error_t account_update_role(
    int32_t account_id,
    const char *new_role
) {
    if (account_id <= 0 || !new_role) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Build SQL filter
    char filter[128];
    snprintf(filter, sizeof(filter), "id = %d", account_id);

    // Create payload
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "role", new_role);

    cJSON *response = NULL;
    db_error_t err = db_patch("accounts", filter, payload, &response);
    
    cJSON_Delete(payload);
    if (response) cJSON_Delete(response);
    
    return err;
}

bool account_email_exists(const char *email) {
    if (!email) {
        return false;
    }

    account_t *account = NULL;
    db_error_t err = account_find_by_email(email, &account);
    
    if (err == DB_SUCCESS && account) {
        account_free(account);
        return true;
    }
    
    return false;
}

void account_free(account_t *account) {
    if (!account) return;

    free(account->email);
    free(account->password);
    free(account->role);
    free(account);
}