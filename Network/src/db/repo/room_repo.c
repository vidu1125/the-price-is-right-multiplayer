#include "db/repo/room_repo.h"
#include "db/models/model.h"      // room_t
#include "db/core/db_client.h"    // db_get, db_error_t, DB_OK
#include <cjson/cJSON.h>          // cJSON
#include <string.h>
#include <stdio.h>

int room_repo_create(
    const char *name,
    uint8_t visibility,
    uint8_t mode,
    uint8_t max_players,
    uint8_t round_time,
    uint8_t wager_enabled,
    char *out_buf,
    size_t out_size,
    uint32_t *room_id
) {
    (void)name;
    (void)visibility;
    (void)mode;
    (void)max_players;
    (void)round_time;
    (void)wager_enabled;

    *room_id = 1;

    const char *json = "{\"success\":true,\"room_id\":1}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';

    return 0;
}

int room_repo_close(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
) {
    (void)room_id;

    const char *json = "{\"success\":true}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';

    return 0;
}
int room_repo_set_rules(
    uint32_t room_id,
    uint8_t mode,
    uint8_t max_players,
    uint8_t round_time,
    uint8_t wager_enabled,
    char *out_buf,
    size_t out_size
) {
    (void)room_id;
    (void)mode;
    (void)max_players;
    (void)round_time;
    (void)wager_enabled;

    const char *json = "{\"success\":true}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';

    return 0;
}
int room_repo_kick_member(
    uint32_t room_id,
    uint32_t target_id,
    char *out_buf,
    size_t out_size
) {
    (void)room_id;
    (void)target_id;

    const char *json = "{\"success\":true}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';

    return 0;
}

int room_repo_leave(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
) {
    (void)room_id;

    const char *json = "{\"success\":true}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';

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
    out_room->round_time =
        cJSON_GetObjectItem(item, "round_time")->valueint;
    out_room->wager_mode =
        cJSON_IsTrue(cJSON_GetObjectItem(item, "wager_mode"));

    cJSON_Delete(json);
    return 0;
}