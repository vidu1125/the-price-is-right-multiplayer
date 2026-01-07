#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cjson/cJSON.h>

#include "db/repo/match_repo.h"
#include "db/core/db_client.h"
#include "handlers/history_handler.h"

// Helper to convert ISO8601 string to unix timestamp
static uint64_t parse_iso8601(const char *date_str) {
    if (!date_str) return 0;
    struct tm tm = {0};
    // simple parsing for "YYYY-MM-DDTHH:MM:SS"
    if (sscanf(date_str, "%d-%d-%dT%d:%d:%d", 
               &tm.tm_year, &tm.tm_mon, &tm.tm_mday, 
               &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) {
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        return (uint64_t)mktime(&tm);
    }
    return 0;
}

// Helper to get current ISO8601 time
static void get_current_iso_time(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    strftime(buf, size, "%Y-%m-%dT%H:%M:%SZ", t);
}

// Helper to get any valid room ID or create a dummy one
static uint32_t get_or_create_dummy_room(int32_t host_id) {
    // 1. Try to find any room
    cJSON *res = NULL;
    db_error_t err = db_get("rooms", "select=id&limit=1", &res);
    if (err == DB_SUCCESS && cJSON_IsArray(res) && cJSON_GetArraySize(res) > 0) {
        cJSON *item = cJSON_GetArrayItem(res, 0);
        cJSON *id_json = cJSON_GetObjectItem(item, "id");
        int room_id = id_json ? id_json->valueint : 0;
        cJSON_Delete(res);
        return room_id;
    }
    if (res) cJSON_Delete(res);

    // 2. Create dummy room
    // Need a valid host_id. use the account_id passed in.
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "name", "History Seed Room");
    cJSON_AddStringToObject(payload, "code", "SEED123"); 
    cJSON_AddStringToObject(payload, "visibility", "public");
    cJSON_AddNumberToObject(payload, "host_id", host_id);
    cJSON_AddStringToObject(payload, "status", "ended");
    
    // Auto timestamp
    char now[32];
    get_current_iso_time(now, sizeof(now));
    cJSON_AddStringToObject(payload, "created_at", now);

    err = db_post("rooms", payload, &res);
    cJSON_Delete(payload);

    uint32_t new_id = 0;
    if (err == DB_SUCCESS && cJSON_IsArray(res) && cJSON_GetArraySize(res) > 0) {
         cJSON *item = cJSON_GetArrayItem(res, 0);
         cJSON *id_json = cJSON_GetObjectItem(item, "id");
         if (id_json) new_id = id_json->valueint;
    }
    if (res) cJSON_Delete(res);
    
    return new_id;
}

db_error_t db_match_create(
    uint32_t room_id,
    int32_t account_id,
    int32_t score,
    const char *mode,
    const char *ranking,
    bool is_winner,
    uint32_t *out_match_id
) {
    if (account_id <= 0) return DB_ERROR_INVALID_PARAM;
    (void)ranking; // Not stored in DB

    // Ensure valid room_id
    if (room_id == 0) {
        room_id = get_or_create_dummy_room(account_id);
        if (room_id == 0) {
             printf("[DB] Failed to resolve room_id for match creation\n");
             return DB_ERROR_INVALID_PARAM; 
        }
    }

    // 1. Insert into matches
    cJSON *match_payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(match_payload, "room_id", room_id);
    cJSON_AddStringToObject(match_payload, "mode", mode ? mode : "Scoring");
    
    char now_str[32];
    get_current_iso_time(now_str, sizeof(now_str));
    cJSON_AddStringToObject(match_payload, "started_at", now_str);
    cJSON_AddStringToObject(match_payload, "ended_at", now_str); // Finished immediately for seed

    cJSON *match_res = NULL;
    db_error_t err = db_post("matches", match_payload, &match_res);
    cJSON_Delete(match_payload);

    if (err != DB_SUCCESS) {
        if (match_res) cJSON_Delete(match_res);
        return err;
    }

    // Get Match ID
    uint32_t match_id = 0;
    if (cJSON_IsArray(match_res) && cJSON_GetArraySize(match_res) > 0) {
        cJSON *item = cJSON_GetArrayItem(match_res, 0);
        cJSON *id_json = cJSON_GetObjectItem(item, "id");
        if (id_json) match_id = id_json->valueint;
    }
    cJSON_Delete(match_res);

    if (match_id == 0) return DB_ERROR_PARSE;
    if (out_match_id) *out_match_id = match_id;

    // 2. Insert into match_players
    cJSON *player_payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(player_payload, "match_id", match_id);
    cJSON_AddNumberToObject(player_payload, "account_id", account_id);
    cJSON_AddNumberToObject(player_payload, "score", score);
    cJSON_AddBoolToObject(player_payload, "winner", is_winner);
    // eliminated, forfeited defaults false
    
    cJSON *player_res = NULL;
    err = db_post("match_players", player_payload, &player_res);
    cJSON_Delete(player_payload);

    if (player_res) cJSON_Delete(player_res);
    
    return err;
}

db_error_t db_match_get_history(
    int32_t account_id,
    int limit,
    int offset,
    HistoryRecord **out_records,
    int *out_count
) {
    if (account_id <= 0 || !out_records || !out_count) return DB_ERROR_INVALID_PARAM;
    
    *out_count = 0;
    *out_records = NULL;

    // Query match_players, embed matches
    // select=score,winner,matches(id,mode,ended_at) & account_id=eq.x
    char query[512];
    snprintf(query, sizeof(query), 
        "select=score,winner,match_id,matches(id,mode,ended_at)&account_id=eq.%d&order=id.desc&limit=%d&offset=%d",
        account_id, limit, offset);

    cJSON *response = NULL;
    db_error_t err = db_get("match_players", query, &response);

    if (err != DB_SUCCESS) {
        if (response) cJSON_Delete(response);
        return err;
    }

    if (!cJSON_IsArray(response)) {
        cJSON_Delete(response);
        return DB_ERROR_PARSE;
    }

    int count = cJSON_GetArraySize(response);
    if (count == 0) {
        cJSON_Delete(response);
        return DB_SUCCESS; // Empty success
    }

    HistoryRecord *records = calloc(count, sizeof(HistoryRecord));
    if (!records) {
        cJSON_Delete(response);
        return DB_ERROR_INTERNAL;
    }

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(response, i);
        
        cJSON *score_json = cJSON_GetObjectItem(item, "score");
        cJSON *winner_json = cJSON_GetObjectItem(item, "winner");
        cJSON *mid_json = cJSON_GetObjectItem(item, "match_id"); // direct column
        
        cJSON *matches_obj = cJSON_GetObjectItem(item, "matches");
        cJSON *mode_json = matches_obj ? cJSON_GetObjectItem(matches_obj, "mode") : NULL;
        cJSON *date_json = matches_obj ? cJSON_GetObjectItem(matches_obj, "ended_at") : NULL;

        if (mid_json) records[i].match_id = mid_json->valueint;
        if (score_json) records[i].final_score = score_json->valueint;
        if (winner_json) records[i].is_winner = cJSON_IsTrue(winner_json) ? 1 : 0;

        // Map mode
        records[i].mode = 0;
        if (mode_json && cJSON_IsString(mode_json)) {
            const char *m = mode_json->valuestring;
            if (strcasecmp(m, "Scoring") == 0) records[i].mode = 1;
            else if (strcasecmp(m, "Eliminated") == 0) records[i].mode = 2;
        }

        // Ranking - not stored, default to "-"
        strcpy(records[i].ranking, "-");

        if (date_json && cJSON_IsString(date_json)) {
            records[i].ended_at = parse_iso8601(date_json->valuestring);
        }
    }

    *out_records = records;
    *out_count = count;
    
    cJSON_Delete(response);
    return DB_SUCCESS;
}

int match_repo_start(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
) {
    (void)room_id;

    snprintf(
        out_buf,
        out_size,
        "{\"success\":true,\"message\":\"game started\"}"
    );

    return 0;
}