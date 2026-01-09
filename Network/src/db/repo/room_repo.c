#include "db/repo/room_repo.h"
#include "db/models/model.h"      // room_t
#include "db/core/db_client.h"    // db_get, db_post, db_error_t, DB_OK
#include <cjson/cJSON.h>          // cJSON
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>                 // time()

//==============================================================================
// HELPER: Generate random 6-char room code
//==============================================================================
static void generate_room_code(char *out_code) {
    // srand(time(NULL) + (unsigned long)out_code);
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < 6; i++) {
        out_code[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    out_code[6] = '\0';
}

//==============================================================================
// CREATE ROOM
//==============================================================================
int room_repo_create(
    const char *name,
    uint8_t visibility,
    uint8_t mode,
    uint8_t max_players,
    uint8_t wager_enabled,
    int32_t host_account_id,
    char *out_buf,
    size_t out_size,
    uint32_t *room_id
) {
    // 1. Generate unique room code
    char code[7];
    generate_room_code(code);
    
    // 2. Build JSON payload
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "name", name);
    cJSON_AddStringToObject(payload, "code", code);
    cJSON_AddStringToObject(payload, "visibility", visibility ? "private" : "public");
    cJSON_AddNumberToObject(payload, "host_id", host_account_id);
    cJSON_AddStringToObject(payload, "status", "waiting");
    cJSON_AddStringToObject(payload, "mode", mode ? "elimination" : "scoring");
    cJSON_AddNumberToObject(payload, "max_players", max_players);
    cJSON_AddBoolToObject(payload, "wager_mode", wager_enabled);
    
    printf("[ROOM_REPO] Creating room: name='%s', code='%s', host_id=%d\n", name, code, host_account_id);
    
    // 3. POST to Supabase
    cJSON *response = NULL;
    db_error_t rc = db_post("rooms", payload, &response);
    cJSON_Delete(payload);
    
    printf("[ROOM_REPO] db_post returned rc=%d, response=%p\n", rc, (void*)response);
    
    if (rc != DB_OK || !response) {
        printf("[ROOM_REPO] ERROR: Database POST failed\n");
        if (response) cJSON_Delete(response);
        snprintf(out_buf, out_size, "{\"success\":false,\"error\":\"Database error\"}");
        return -1;
    }
    
    // 4. Parse response - Supabase returns array with 1 object
    cJSON *first = cJSON_GetArrayItem(response, 0);
    if (!first) {
        cJSON_Delete(response);
        snprintf(out_buf, out_size, "{\"success\":false,\"error\":\"Invalid response\"}");
        return -1;
    }
    
    cJSON *id_item = cJSON_GetObjectItem(first, "id");
    cJSON *code_item = cJSON_GetObjectItem(first, "code");
    
    if (!id_item || !code_item) {
        cJSON_Delete(response);
        snprintf(out_buf, out_size, "{\"success\":false,\"error\":\"Missing fields\"}");
        return -1;
    }
    
    *room_id = (uint32_t)id_item->valueint;
    
    // 5. Insert host into room_members table
    cJSON *member_payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(member_payload, "room_id", *room_id);
    cJSON_AddNumberToObject(member_payload, "account_id", host_account_id);
    
    cJSON *member_resp = NULL;
    db_error_t member_rc = db_post("room_members", member_payload, &member_resp);
    cJSON_Delete(member_payload);
    if (member_resp) cJSON_Delete(member_resp);
    
    if (member_rc != DB_OK) {
        cJSON_Delete(response);
        snprintf(out_buf, out_size, "{\"success\":false,\"error\":\"Failed to add host to room\"}");
        return -1;
    }
    
    // 6. Build success response
    cJSON *name_item = cJSON_GetObjectItem(first, "name");
    const char *room_name = name_item ? name_item->valuestring : "Room";
    
    snprintf(out_buf, out_size, 
        "{\"success\":true,\"room_id\":%d,\"room_code\":\"%s\",\"room_name\":\"%s\"}",
        *room_id, code_item->valuestring, room_name);
    
    cJSON_Delete(response);
    return 0;
}

//==============================================================================
// CLOSE ROOM (UPDATE status to 'closed')
//==============================================================================
int room_repo_close(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
) {
    // Use RPC to update status and clear members
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "p_room_id", room_id);
    
    cJSON *response = NULL;
    db_error_t rc = db_rpc("close_room", payload, &response);
    
    cJSON_Delete(payload);
    
    if (rc != DB_OK) {
        if (response) cJSON_Delete(response);
        snprintf(out_buf, out_size, "{\"success\":false,\"error\":\"Close failed\"}");
        return -1;
    }
    
    snprintf(out_buf, out_size, "{\"success\":true}");
    if (response) cJSON_Delete(response);
    return 0;
}
//==============================================================================
// SET RULES (UPDATE room settings)
//==============================================================================
int room_repo_set_rules(
    uint32_t room_id,
    uint8_t mode,
    uint8_t max_players,
    uint8_t wager_enabled,
    uint8_t visibility,
    char *out_buf,
    size_t out_size
) {
    // Build update payload
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "mode", mode ? "elimination" : "scoring");
    cJSON_AddNumberToObject(payload, "max_players", max_players);
    cJSON_AddBoolToObject(payload, "wager_mode", wager_enabled);
    
    // PATCH /rooms?id=eq.{room_id}
    char query[128];
    snprintf(query, sizeof(query), "id=eq.%u", room_id);
    
    // Note: db_client doesn't have PATCH yet, use POST to RPC or build custom
    // For now, use workaround: DELETE old + INSERT new is bad
    // Better: Add db_patch() to db_client
    
    // Workaround: Use RPC function
    cJSON *rpc_payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(rpc_payload, "p_room_id", room_id);
    cJSON_AddStringToObject(rpc_payload, "p_mode", mode ? "elimination" : "scoring");
    cJSON_AddNumberToObject(rpc_payload, "p_max_players", max_players);
    cJSON_AddBoolToObject(rpc_payload, "p_wager_mode", wager_enabled);
    cJSON_AddStringToObject(rpc_payload, "p_visibility", visibility ? "private" : "public");
    
    cJSON *response = NULL;
    db_error_t rc = db_rpc("update_room_rules", rpc_payload, &response);
    
    cJSON_Delete(payload);
    cJSON_Delete(rpc_payload);
    
    if (rc != DB_OK) {
        if (response) cJSON_Delete(response);
        snprintf(out_buf, out_size, "{\"success\":false,\"error\":\"Failed to update\"}" );
        return -1;
    }
    
    snprintf(out_buf, out_size, "{\"success\":true}");
    if (response) cJSON_Delete(response);
    return 0;
}
//==============================================================================
// KICK MEMBER (DELETE from room_members)
//==============================================================================
int room_repo_kick_member(
    uint32_t room_id,
    uint32_t target_id,
    char *out_buf,
    size_t out_size
) {
    // Use RPC to delete member
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "p_room_id", room_id);
    cJSON_AddNumberToObject(payload, "p_account_id", target_id);
    
    cJSON *response = NULL;
    db_error_t rc = db_rpc("kick_member", payload, &response);
    
    cJSON_Delete(payload);
    
    if (rc != DB_OK) {
        if (response) cJSON_Delete(response);
        snprintf(out_buf, out_size, "{\"success\":false,\"error\":\"Kick failed\"}");
        return -1;
    }
    
    snprintf(out_buf, out_size, "{\"success\":true}");
    if (response) cJSON_Delete(response);
    return 0;
}

//==============================================================================
// LEAVE ROOM (DELETE self from room_members)
//==============================================================================
int room_repo_leave(
    uint32_t room_id,
    int32_t account_id,
    char *out_buf,
    size_t out_size
) {
    // Use RPC to leave
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "p_room_id", room_id);
    cJSON_AddNumberToObject(payload, "p_account_id", account_id);
    
    cJSON *response = NULL;
    db_error_t rc = db_rpc("leave_room", payload, &response);
    
    cJSON_Delete(payload);
    
    if (rc != DB_OK) {
        if (response) cJSON_Delete(response);
        snprintf(out_buf, out_size, "{\"success\":false,\"error\":\"Leave failed\"}");
        return -1;
    }
    
    snprintf(out_buf, out_size, "{\"success\":true}");
    if (response) cJSON_Delete(response);
    return 0;
}

//==============================================================================
// GET ROOM STATE (Query DB for rules + members)
//==============================================================================
int room_repo_get_state(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
) {
    // Use RPC to get room state
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "p_room_id", room_id);
    
    cJSON *response = NULL;
    db_error_t rc = db_rpc("get_room_state", payload, &response);
    
    cJSON_Delete(payload);
    
    if (rc != DB_OK || !response) {
        return -1;
    }
    
    // Convert response to string
    char *json_str = cJSON_PrintUnformatted(response);
    if (json_str) {
        snprintf(out_buf, out_size, "%s", json_str);
        free(json_str);
    }
    
    cJSON_Delete(response);
    return 0;
}

int room_repo_get_by_code(const char *code, room_t *out_room) {
    if (!code || !out_room) return -1;

    char query[128];
    snprintf(query, sizeof(query),
             "code=eq.%s&select=*", code);

    cJSON *json = NULL;
    db_error_t rc = db_get("rooms", query, &json);
    if (rc != DB_OK || !json) return -1;

    // Supabase trả về ARRAY
    cJSON *item = cJSON_GetArrayItem(json, 0);
    if (!item) {
        cJSON_Delete(json);
        return -1;
    }

    // map JSON → struct
    out_room->id = cJSON_GetObjectItem(item, "id")->valueint;
    strcpy(out_room->name,
           cJSON_GetObjectItem(item, "name")->valuestring);
    strcpy(out_room->code,
           cJSON_GetObjectItem(item, "code")->valuestring);
    strcpy(out_room->visibility,
           cJSON_GetObjectItem(item, "visibility")->valuestring);
    strcpy(out_room->status,
           cJSON_GetObjectItem(item, "status")->valuestring);
    strcpy(out_room->mode,
           cJSON_GetObjectItem(item, "mode")->valuestring);

    out_room->host_id =
        cJSON_GetObjectItem(item, "host_id")->valueint;
    out_room->max_players =
        cJSON_GetObjectItem(item, "max_players")->valueint;
    out_room->wager_mode =
        cJSON_IsTrue(cJSON_GetObjectItem(item, "wager_mode"));

    cJSON_Delete(json);
    return 0;
}