#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "db/repo/session_repo.h"
#include "db/core/db_client.h"
#include "utils/crypto.h"

// Helper function to parse session from JSON
static session_t* parse_session_from_json(cJSON *json) {
    if (!json) return NULL;

    session_t *session = calloc(1, sizeof(session_t));
    if (!session) return NULL;

    cJSON *account_id = cJSON_GetObjectItem(json, "account_id");
    cJSON *session_id = cJSON_GetObjectItem(json, "session_id");
    cJSON *connected = cJSON_GetObjectItem(json, "connected");
    (void)cJSON_GetObjectItem(json, "updated_at");

    if (account_id && cJSON_IsNumber(account_id)) {
        session->account_id = account_id->valueint;
    }

    if (session_id && cJSON_IsString(session_id)) {
        strncpy(session->session_id, session_id->valuestring, 36);
        session->session_id[36] = '\0';
    }

    if (connected && cJSON_IsBool(connected)) {
        session->connected = cJSON_IsTrue(connected);
    }

    // TODO: Parse timestamp if needed
    session->updated_at = 0;

    return session;
}

db_error_t session_create(
    int32_t account_id,
    session_t **out_session
) {
    if (account_id <= 0 || !out_session) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Generate new UUID
    char uuid[37];
    if (!crypto_generate_uuid(uuid)) {
        return DB_ERROR_INTERNAL;
    }

    // Create JSON payload
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "account_id", account_id);
    cJSON_AddStringToObject(payload, "session_id", uuid);
    cJSON_AddBoolToObject(payload, "connected", true);

    cJSON *response = NULL;
    db_error_t err = db_post("sessions", payload, &response);
    cJSON_Delete(payload);

    if (err != DB_SUCCESS) {
        if (response) cJSON_Delete(response);
        return err;
    }

    // Parse response
    if (cJSON_IsArray(response) && cJSON_GetArraySize(response) > 0) {
        cJSON *item = cJSON_GetArrayItem(response, 0);
        *out_session = parse_session_from_json(item);
        cJSON_Delete(response);
        return *out_session ? DB_SUCCESS : DB_ERROR_PARSE;
    }

    cJSON_Delete(response);
    return DB_ERROR_PARSE;
}

db_error_t session_find_by_id(
    const char *session_id,
    session_t **out_session
) {
    if (!session_id || !out_session) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Build query: select=*&session_id=eq.{session_id}&limit=1
    char query[256];
    snprintf(query, sizeof(query), "select=*&session_id=eq.%s&limit=1", session_id);

    cJSON *response = NULL;
    db_error_t err = db_get("sessions", query, &response);

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
        *out_session = parse_session_from_json(item);
        cJSON_Delete(response);
        return *out_session ? DB_SUCCESS : DB_ERROR_PARSE;
    }

    cJSON_Delete(response);
    return DB_ERROR_PARSE;
}

db_error_t session_find_by_account(
    int32_t account_id,
    session_t **out_session
) {
    if (account_id <= 0 || !out_session) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Build query: select=*&account_id=eq.{account_id}&limit=1
    char query[256];
    snprintf(query, sizeof(query), "select=*&account_id=eq.%d&limit=1", account_id);

    cJSON *response = NULL;
    db_error_t err = db_get("sessions", query, &response);

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
        *out_session = parse_session_from_json(item);
        cJSON_Delete(response);
        return *out_session ? DB_SUCCESS : DB_ERROR_PARSE;
    }

    cJSON_Delete(response);
    return DB_ERROR_PARSE;
}

db_error_t session_update_connected(
    const char *session_id,
    bool connected
) {
    if (!session_id) {
        return DB_ERROR_INVALID_PARAM;
    }

    printf("[SESSION_REPO] Updating session %s connected=%d\n", session_id, connected);

    // Build filter: session_id=eq.{session_id}
    char filter[128];
    snprintf(filter, sizeof(filter), "session_id=eq.%s", session_id);

    // Create payload
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddBoolToObject(payload, "connected", connected);

    cJSON *response = NULL;
    db_error_t err = db_patch("sessions", filter, payload, &response);
    
    printf("[SESSION_REPO] PATCH result: err=%d\n", err);
    
    cJSON_Delete(payload);
    if (response) cJSON_Delete(response);
    
    return err;
}

db_error_t session_reconnect(
    const char *session_id
) {
    return session_update_connected(session_id, true);
}

db_error_t session_delete(
    const char *session_id
) {
    if (!session_id) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Build filter: session_id=eq.{session_id}
    char filter[128];
    snprintf(filter, sizeof(filter), "session_id=eq.%s", session_id);

    cJSON *response = NULL;
    db_error_t err = db_delete("sessions", filter, &response);
    
    if (response) cJSON_Delete(response);
    
    return err;
}

db_error_t session_delete_by_account(
    int32_t account_id
) {
    if (account_id <= 0) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Build filter: account_id=eq.{account_id}
    char filter[128];
    snprintf(filter, sizeof(filter), "account_id=eq.%d", account_id);

    cJSON *response = NULL;
    db_error_t err = db_delete("sessions", filter, &response);
    
    if (response) cJSON_Delete(response);
    
    return err;
}

bool session_is_valid(const char *session_id) {
    if (!session_id) {
        return false;
    }

    session_t *session = NULL;
    db_error_t err = session_find_by_id(session_id, &session);
    
    if (err == DB_SUCCESS && session && session->connected) {
        session_free(session);
        return true;
    }
    
    if (session) {
        session_free(session);
    }
    
    return false;
}

void session_free(session_t *session) {
    if (!session) return;
    free(session);
}