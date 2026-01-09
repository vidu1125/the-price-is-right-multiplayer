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