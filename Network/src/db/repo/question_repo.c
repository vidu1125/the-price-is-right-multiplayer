#include <stddef.h>
#include <string.h>
#include "db/repo/question_repo.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "db/core/db_client.h"
int history_repo_get(
   int32_t account_id,
    cJSON **out_json
) {
    if (!out_json) return -1;

    *out_json = NULL;

    /**
     * Ý tưởng query:
     * 1. Lấy 10 match gần nhất mà account tham gia
     * 2. Join sang match_players → match_answers → match_question
     */
    const char *sql =
        "WITH recent_matches AS ( "
        "   SELECT mp.id AS player_id, mp.match_id "
        "   FROM match_players mp "
        "   WHERE mp.account_id = %d "
        "   ORDER BY mp.joined_at DESC "
        "   LIMIT 10 "
        ") "
        "SELECT "
        "   rm.match_id, "
        "   mq.round_no, "
        "   mq.round_type, "
        "   mq.question, "
        "   ma.answer, "
        "   ma.score_delta, "
        "   ma.created_at "
        "FROM recent_matches rm "
        "JOIN match_answers ma "
        "   ON ma.player_id = rm.player_id "
        "JOIN match_question mq "
        "   ON mq.id = ma.question_id "
        "ORDER BY rm.match_id DESC, mq.round_no ASC;";

    char query[2048];
    snprintf(query, sizeof(query), sql, account_id);

    cJSON *result = NULL;
    db_error_t rc = db_get("query", query, &result);

    if (rc != DB_OK || !result) {
        printf("[repo][question] query failed rc=%d\n", rc);
        return -1;
    }

    if (!cJSON_IsArray(result)) {
        cJSON_Delete(result);
        return -1;
    }

    *out_json = result;
    return 0;
}

// Get question IDs that appeared in recent N matches of given players
db_error_t question_get_excluded_ids(
    const int32_t *account_ids,
    int player_count,
    int recent_match_count,
    int32_t **out_excluded_ids,
    int *out_excluded_count
) {
    if (!account_ids || player_count <= 0 || !out_excluded_ids || !out_excluded_count) {
        return DB_ERROR_INVALID_PARAM;
    }

    *out_excluded_ids = NULL;
    *out_excluded_count = 0;

    // Build query to get question IDs from recent matches
    // For each player, get their N most recent matches, then get all question IDs from those matches
    char query[2048];
    int offset = 0;
    
    // Build list of account IDs for SQL IN clause
    char account_list[256] = "";
    for (int i = 0; i < player_count; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d%s", account_ids[i], (i < player_count - 1) ? "," : "");
        strcat(account_list, buf);
    }

    // SQL query: Get distinct question IDs from match_question table
    // Join with match_players to filter by account_ids
    snprintf(query, sizeof(query),
        "SELECT DISTINCT mq.question->>'id' as question_id "
        "FROM match_question mq "
        "JOIN matches m ON mq.match_id = m.id "
        "JOIN match_players mp ON mp.match_id = m.id "
        "WHERE mp.account_id IN (%s) "
        "ORDER BY m.started_at DESC "
        "LIMIT %d",
        account_list, recent_match_count * 10);  // Get more to ensure coverage

    cJSON *result = NULL;
    db_error_t rc = db_get("match_question", query, &result);

    if (rc != DB_OK || !result) {
        printf("[QUESTION_REPO] No recent matches found or query failed\n");
        return DB_OK; // Not an error, just no exclusions
    }

    if (!cJSON_IsArray(result)) {
        cJSON_Delete(result);
        return DB_OK;
    }

    int count = cJSON_GetArraySize(result);
    if (count == 0) {
        cJSON_Delete(result);
        return DB_OK;
    }

    // Allocate array for excluded IDs
    int32_t *excluded = malloc(count * sizeof(int32_t));
    if (!excluded) {
        cJSON_Delete(result);
        return DB_ERROR_INTERNAL;
    }

    // Extract question IDs from result
    int valid_count = 0;
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);
        if (!item) continue;
        
        // Result should have question_id field from our SQL
        cJSON *id_field = cJSON_GetObjectItem(item, "question_id");
        if (id_field) {
            if (cJSON_IsNumber(id_field)) {
                excluded[valid_count++] = id_field->valueint;
            } else if (cJSON_IsString(id_field)) {
                excluded[valid_count++] = atoi(id_field->valuestring);
            }
        }
    }

    cJSON_Delete(result);

    *out_excluded_ids = excluded;
    *out_excluded_count = valid_count;

    printf("[QUESTION_REPO] Excluding %d question IDs from selection\n", valid_count);
    return DB_OK;
}

// Fetch random questions from database filtered by round type
db_error_t question_get_random(const char *round_type, int count, const int32_t *excluded_ids, int excluded_count, const char *category, cJSON **out_json) {
    if (!out_json || count <= 0 || !round_type) {
        return DB_ERROR_INVALID_PARAM;
    }

    *out_json = NULL;

    // Build SQL query with JSONB operators
    char query[512];
    
    if (category && strlen(category) > 0) {
        // Filter by both round type AND category in JSONB data
        snprintf(query, sizeof(query), 
                 "SELECT * FROM questions WHERE type = '%s' AND data->>'category' = '%s' AND active = true",
                 round_type, category);
        printf("[QUESTION_REPO] Fetching questions for type='%s', category='%s' (excluding %d IDs)\n", 
               round_type, category, excluded_count);
    } else {
        // Only filter by round type (all categories)
        snprintf(query, sizeof(query), 
                 "SELECT * FROM questions WHERE type = '%s' AND active = true",
                 round_type);
        printf("[QUESTION_REPO] Fetching questions for type='%s' (all categories, excluding %d IDs)\n", 
               round_type, excluded_count);
    }

    cJSON *result = NULL;
    db_error_t rc = db_get("questions", query, &result);

    if (rc != DB_OK) {
        printf("[QUESTION_REPO] Query failed: rc=%d\n", rc);
        return rc;
    }

    if (!result || !cJSON_IsArray(result)) {
        printf("[QUESTION_REPO] Invalid response format (expected array)\n");
        if (result) cJSON_Delete(result);
        return DB_ERROR_PARSE;
    }

    int total = cJSON_GetArraySize(result);
    printf("[QUESTION_REPO] Found %d total questions\n", total);

    // Filter out excluded questions
    cJSON *filtered = cJSON_CreateArray();
    for (int i = 0; i < total; i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);
        cJSON *id_field = cJSON_GetObjectItem(item, "id");
        if (!id_field || !cJSON_IsNumber(id_field)) continue;

        int32_t q_id = id_field->valueint;
        
        // Check if this ID is in exclusion list
        bool excluded = false;
        for (int j = 0; j < excluded_count; j++) {
            if (excluded_ids[j] == q_id) {
                excluded = true;
                break;
            }
        }

        if (!excluded) {
            cJSON_AddItemToArray(filtered, cJSON_Duplicate(item, 1));
        }
    }

    cJSON_Delete(result);

    int available = cJSON_GetArraySize(filtered);
    printf("[QUESTION_REPO] %d questions available after exclusion, picking %d random\n", available, count);

    if (available == 0) {
        cJSON_Delete(filtered);
        printf("[QUESTION_REPO] WARNING: No questions available after filtering!\n");
        return DB_ERROR_NOT_FOUND;
    }

    if (available <= count) {
        // Not enough, return all available
        *out_json = filtered;
        return DB_OK;
    }

    // Pick 'count' random items
    cJSON *final_array = cJSON_CreateArray();
    srand(time(NULL));

    for (int i = 0; i < count; i++) {
        int remaining = cJSON_GetArraySize(filtered);
        if (remaining == 0) break;
        
        int idx = rand() % remaining;
        cJSON *item = cJSON_DetachItemFromArray(filtered, idx);
        cJSON_AddItemToArray(final_array, item);
    }

    cJSON_Delete(filtered);

    *out_json = final_array;
    return DB_OK;
}