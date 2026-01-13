#pragma once
#include <stddef.h>
#include <cjson/cJSON.h>
#include <stdint.h> 
#include "db/core/db_client.h"  

// GET HISTORY
int history_repo_get(
    int32_t account_id,
    cJSON **out_json
);

// Get question IDs from recent N matches of given players (to exclude duplicates)
db_error_t question_get_excluded_ids(
    const int32_t *account_ids,
    int player_count,
    int recent_match_count,
    int32_t **out_excluded_ids,
    int *out_excluded_count
);

// GET RANDOM QUESTIONS for match filtered by round type
db_error_t question_get_random(
    const char *round_type,  // "mcq", "bid", "wheel"
    int count,
    const int32_t *excluded_ids,
    int excluded_count,
    cJSON **out_json
);
