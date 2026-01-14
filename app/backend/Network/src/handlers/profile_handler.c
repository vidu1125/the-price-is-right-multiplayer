#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "handlers/profile_handler.h"
#include "handlers/session_context.h"
#include "db/repo/profile_repo.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"

static void send_response_json(
    int client_fd,
    MessageHeader *req,
    uint16_t code,
    cJSON *obj
) {
    char *json = cJSON_PrintUnformatted(obj);
    if (!json) return;
    forward_response(client_fd, req, code, json, strlen(json));
    free(json);
}

void handle_update_profile(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    // Require bound session
    int32_t account_id = get_client_account(client_fd);
    if (account_id <= 0) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddBoolToObject(err, "success", false);
        cJSON_AddStringToObject(err, "error", "Not logged in");
        send_response_json(client_fd, header, ERR_NOT_LOGGED_IN, err);
        cJSON_Delete(err);
        return;
    }

    cJSON *json = cJSON_Parse(payload);
    if (!json) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddBoolToObject(err, "success", false);
        cJSON_AddStringToObject(err, "error", "Invalid JSON");
        send_response_json(client_fd, header, ERR_BAD_REQUEST, err);
        cJSON_Delete(err);
        return;
    }

    const char *name = NULL;
    const char *avatar = NULL;
    const char *bio = NULL;

    cJSON *name_js = cJSON_GetObjectItem(json, "name");
    cJSON *avatar_js = cJSON_GetObjectItem(json, "avatar");
    cJSON *bio_js = cJSON_GetObjectItem(json, "bio");
    if (name_js && cJSON_IsString(name_js)) name = name_js->valuestring;
    if (avatar_js && cJSON_IsString(avatar_js)) avatar = avatar_js->valuestring;
    if (bio_js && cJSON_IsString(bio_js)) bio = bio_js->valuestring;

    profile_t *updated = NULL;
    db_error_t err = profile_update_by_account(account_id, name, avatar, bio, &updated);
    if (err != DB_SUCCESS || !updated) {
        cJSON *res = cJSON_CreateObject();
        cJSON_AddBoolToObject(res, "success", false);
        cJSON_AddStringToObject(res, "error", "Failed to update profile");
        send_response_json(client_fd, header, ERR_SERVER_ERROR, res);
        cJSON_Delete(res);
        cJSON_Delete(json);
        return;
    }

    // Build success payload
    cJSON *res = cJSON_CreateObject();
    cJSON_AddBoolToObject(res, "success", true);
    cJSON *p = cJSON_CreateObject();
    cJSON_AddNumberToObject(p, "id", updated->id);
    cJSON_AddNumberToObject(p, "account_id", updated->account_id);
    if (updated->name) cJSON_AddStringToObject(p, "name", updated->name);
    if (updated->avatar) cJSON_AddStringToObject(p, "avatar", updated->avatar);
    if (updated->bio) cJSON_AddStringToObject(p, "bio", updated->bio);
    cJSON_AddNumberToObject(p, "matches", updated->matches);
    cJSON_AddNumberToObject(p, "wins", updated->wins);
    cJSON_AddNumberToObject(p, "points", updated->points);
    if (updated->badges) cJSON_AddStringToObject(p, "badges", updated->badges);
    cJSON_AddItemToObject(res, "profile", p);

    send_response_json(client_fd, header, RES_PROFILE_UPDATED, res);

    cJSON_Delete(res);
    cJSON_Delete(json);
    profile_free(updated);
}

void handle_get_profile(
    int client_fd,
    MessageHeader *header,
    const char *payload
) {
    (void)payload; // Not used for GET
    printf("[PROFILE] Processing get_profile request\n");

    // Require bound session
    int32_t account_id = get_client_account(client_fd);
    printf("[PROFILE] Account ID from session: %d\n", account_id);
    
    if (account_id <= 0) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddBoolToObject(err, "success", false);
        cJSON_AddStringToObject(err, "error", "Not logged in");
        send_response_json(client_fd, header, ERR_NOT_LOGGED_IN, err);
        cJSON_Delete(err);
        return;
    }

    profile_t *profile = NULL;
    db_error_t db_err = profile_find_by_account(account_id, &profile);
    
    // Create default if not found
    if (db_err == DB_ERROR_NOT_FOUND) {
        profile_create(account_id, NULL, NULL, NULL, &profile);
        db_err = (profile != NULL) ? DB_SUCCESS : DB_ERR_UNKNOWN;
    }

    if (db_err != DB_SUCCESS || !profile) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddBoolToObject(err, "success", false);
        cJSON_AddStringToObject(err, "error", "Profile not found");
        send_response_json(client_fd, header, ERR_SERVER_ERROR, err);
        cJSON_Delete(err);
        return;
    }

    // Build success payload
    cJSON *res = cJSON_CreateObject();
    cJSON_AddBoolToObject(res, "success", true);
    cJSON *p = cJSON_CreateObject();
    cJSON_AddNumberToObject(p, "id", profile->id);
    cJSON_AddNumberToObject(p, "account_id", profile->account_id);
    if (profile->name) cJSON_AddStringToObject(p, "name", profile->name);
    if (profile->avatar) cJSON_AddStringToObject(p, "avatar", profile->avatar);
    if (profile->bio) cJSON_AddStringToObject(p, "bio", profile->bio);
    cJSON_AddNumberToObject(p, "matches", profile->matches);
    cJSON_AddNumberToObject(p, "wins", profile->wins);
    cJSON_AddNumberToObject(p, "points", profile->points);
    if (profile->badges) cJSON_AddStringToObject(p, "badges", profile->badges);
    cJSON_AddItemToObject(res, "profile", p);

    printf("[PROFILE] Sending profile data for: %s\n", profile->name ? profile->name : "Anonymous");

    send_response_json(client_fd, header, RES_PROFILE_FOUND, res);

    cJSON_Delete(res);
    profile_free(profile);
}