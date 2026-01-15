#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "db/core/db_client.h"
#include "handlers/history_handler.h" // For HistoryRecord

// Creates a new match record in the DB
db_error_t db_match_create(
    uint32_t room_id,
    int32_t account_id,
    int32_t score,
    const char *mode,
    const char *ranking,
    bool is_winner,
    uint32_t *out_match_id
);

// Insert match only (without players) - for game start
db_error_t db_match_insert(
    uint32_t room_id,
    const char *mode,
    int max_players,
    int64_t *out_match_id
);

// Insert match_players for a match
db_error_t db_match_players_insert(
    int64_t db_match_id,
    const int32_t *account_ids,
    int player_count
);

// Fetches history records for an account
db_error_t db_match_get_history(
    int32_t account_id,
    int limit,
    int offset,
    HistoryRecord **out_records,
    int *out_count
);

// Start game logic (Placeholder)
int match_repo_start(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
);

db_error_t db_match_get_detail(
    uint32_t match_id,
    cJSON **out_json
);

// Insert a match event (FORFEIT, ELIMINATED, etc.)
db_error_t db_match_event_insert(
    int64_t db_match_id,
    int32_t player_id,
    const char *event_type,
    int round_no,
    int question_idx
);

// Insert a match_question and return the ID
db_error_t db_match_question_insert(
    int64_t db_match_id,
    int round_no,
    const char *round_type,
    int question_idx,
    const char *question_json,
    int64_t *out_question_id
);

// Insert a match_answer
db_error_t db_match_answer_insert(
    int64_t question_id,
    int32_t player_id,
    const char *answer_json,
    int score_delta,
    int action_idx
);

// Get match_player IDs after inserting match_players
db_error_t db_match_players_get_ids(
    int64_t db_match_id,
    int32_t *account_ids,
    int32_t *out_player_ids,
    int player_count
);
