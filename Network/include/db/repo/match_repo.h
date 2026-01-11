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
