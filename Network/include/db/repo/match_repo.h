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

//==============================================================================
// Match Answer & Event Functions
//==============================================================================

/**
 * Save a player's answer to the database
 * @param match_question_id - ID from match_question table
 * @param match_player_id   - ID from match_players table  
 * @param answer_json       - Answer data as JSON (e.g., {"answer": 2, "is_correct": true})
 * @param score_delta       - Points earned/lost
 * @param action_idx        - Action index (default 0 for MCQ)
 */
db_error_t db_match_save_answer(
    int32_t match_question_id,
    int32_t match_player_id,
    const char *answer_json,
    int32_t score_delta,
    int action_idx
);

/**
 * Save elimination event to database
 * @param match_id        - Match ID
 * @param match_player_id - ID from match_players table
 * @param round_no        - Round number where eliminated (1-based)
 * @param question_idx    - Question index (0 for round-end elimination)
 */
db_error_t db_match_save_event(
    uint32_t match_id,
    int32_t match_player_id,
    const char *event_type,
    int round_no,
    int question_idx
);

/**
 * Create match_question record and return its ID
 * @param match_id     - Match ID
 * @param round_no     - Round number (1-based)
 * @param round_type   - "MCQ", "BID", "WHEEL", "BONUS"
 * @param question_idx - Question index within round (0-based)
 * @param question_json - Question data snapshot
 * @param out_id       - Output: created match_question.id
 */
db_error_t db_match_create_question(
    uint32_t match_id,
    int round_no,
    const char *round_type,
    int question_idx,
    const char *question_json,
    int32_t *out_id
);

/**
 * Update match_players with final score and elimination status
 */
db_error_t db_match_update_player(
    int32_t match_player_id,
    int32_t final_score,
    bool eliminated,
    int eliminated_at_round
);
