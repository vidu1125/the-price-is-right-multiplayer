#include <stddef.h>
#include <string.h>
#include "db/repo/question_repo.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
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

// Fetch random questions from database filtered by round type
db_error_t question_get_random(const char *round_type, int count, cJSON **out_json) {
    if (!out_json || count <= 0 || !round_type) {
        return DB_ERROR_INVALID_PARAM;
    }

    *out_json = NULL;

    // Use PostgREST syntax instead of raw SQL
    // Fetch all questions of this type, then shuffle in C
    // Use PostgREST syntax instead of raw SQL
    // Fetch all questions of this type, then shuffle in C
    char query[256];
    // Guessing column 'type' for filtering. If this fails, we will need schema info.
    snprintf(query, sizeof(query), "select=*&type=eq.%s", round_type);

    printf("[QUESTION_REPO] Fetching questions for type='%s'\n", round_type);

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
    printf("[QUESTION_REPO] Found %d questions, picking %d random\n", total, count);

    if (total <= count) {
        // Not enough or just enough questions, return all
        *out_json = result;
        return DB_OK;
    }

    // Pick 'count' random items
    cJSON *final_array = cJSON_CreateArray();
    srand(time(NULL)); // Simple seeding

    for (int i = 0; i < count; i++) {
        // Pick a random index from remaining items
        int remaining = cJSON_GetArraySize(result);
        if (remaining == 0) break;
        
        int idx = rand() % remaining;
        cJSON *item = cJSON_DetachItemFromArray(result, idx);
        cJSON_AddItemToArray(final_array, item);
    }

    // Clean up the rest
    cJSON_Delete(result);

    *out_json = final_array;
    return DB_OK;
}
