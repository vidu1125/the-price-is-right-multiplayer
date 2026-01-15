#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cjson/cJSON.h>
#include <stdbool.h>

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
    db_error_t err = db_get("rooms", "SELECT id FROM rooms LIMIT 1", &res);
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

// Insert match only - for game start
db_error_t db_match_insert(
    uint32_t room_id,
    const char *mode,
    int max_players,
    int64_t *out_match_id
) {
    if (!out_match_id) return DB_ERROR_INVALID_PARAM;
    *out_match_id = 0;

    // Use hardcoded room_id for testing (will be replaced with actual room data later)
    // Build match payload
    cJSON *match_payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(match_payload, "room_id", room_id);
    cJSON_AddStringToObject(match_payload, "mode", mode ? mode : "classic");
    cJSON_AddNumberToObject(match_payload, "max_players", max_players);
    
    // started_at = now
    char now_str[32];
    get_current_iso_time(now_str, sizeof(now_str));
    cJSON_AddStringToObject(match_payload, "started_at", now_str);
    // ended_at = null (match not ended yet)

    cJSON *match_res = NULL;
    db_error_t err = db_post("matches", match_payload, &match_res);
    cJSON_Delete(match_payload);

    if (err != DB_SUCCESS) {
        printf("[MATCH_REPO] Failed to insert match: err=%d\n", err);
        if (match_res) cJSON_Delete(match_res);
        return err;
    }

    // Extract match ID from response
    int64_t match_id = 0;
    if (cJSON_IsArray(match_res) && cJSON_GetArraySize(match_res) > 0) {
        cJSON *item = cJSON_GetArrayItem(match_res, 0);
        cJSON *id_json = cJSON_GetObjectItem(item, "id");
        if (id_json && cJSON_IsNumber(id_json)) {
            match_id = id_json->valueint;
        }
    }
    cJSON_Delete(match_res);

    if (match_id == 0) {
        printf("[MATCH_REPO] Failed to parse match ID from response\n");
        return DB_ERROR_PARSE;
    }

    *out_match_id = match_id;
    printf("[MATCH_REPO] Match inserted successfully: db_match_id=%lld\n", (long long)match_id);
    
    return DB_SUCCESS;
}

// Insert match_players for a match
db_error_t db_match_players_insert(
    int64_t db_match_id,
    const int32_t *account_ids,
    int player_count
) {
    if (db_match_id == 0 || !account_ids || player_count <= 0) {
        return DB_ERROR_INVALID_PARAM;
    }

    printf("[MATCH_REPO] Inserting %d players for match_id=%lld...\n", 
           player_count, (long long)db_match_id);

    // Insert players one by one
    for (int i = 0; i < player_count; i++) {
        cJSON *player = cJSON_CreateObject();
        cJSON_AddNumberToObject(player, "match_id", db_match_id);
        cJSON_AddNumberToObject(player, "account_id", account_ids[i]);
        cJSON_AddNumberToObject(player, "score", 0); // Initial score
        cJSON_AddBoolToObject(player, "winner", false);
        cJSON_AddBoolToObject(player, "eliminated", false);
        cJSON_AddBoolToObject(player, "forfeited", false);
        
        cJSON *response = NULL;
        db_error_t err = db_post("match_players", player, &response);
        cJSON_Delete(player);

        if (err != DB_SUCCESS) {
            printf("[MATCH_REPO] Failed to insert player %d: err=%d\n", account_ids[i], err);
            if (response) cJSON_Delete(response);
            return err;
        }
        if (response) cJSON_Delete(response);
    }
    
    printf("[MATCH_REPO] Successfully inserted %d match_players\n", player_count);
    return DB_SUCCESS;
}

db_error_t db_match_get_history(
    int32_t account_id,
    int limit,
    int offset,
    HistoryRecord **out_records,
    int *out_count
) {
    if (account_id <= 0 || !out_records || !out_count)
        return DB_ERROR_INVALID_PARAM;

    *out_count = 0;
    *out_records = NULL;

    // SQL query with JOIN
    char query[512];
    snprintf(
        query,
        sizeof(query),
        "SELECT mp.score, mp.rank, mp.winner, mp.match_id, "
        "m.id as match_id, m.mode, m.ended_at "
        "FROM match_players mp "
        "JOIN matches m ON mp.match_id = m.id "
        "WHERE mp.account_id = %d "
        "ORDER BY mp.id DESC "
        "LIMIT %d OFFSET %d",
        account_id,
        limit,
        offset
    );

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

        cJSON *score_json  = cJSON_GetObjectItem(item, "score");
        cJSON *rank_json   = cJSON_GetObjectItem(item, "rank");
        cJSON *winner_json = cJSON_GetObjectItem(item, "winner");
        cJSON *mid_json    = cJSON_GetObjectItem(item, "match_id");

        // Flat JSON from SQL query, no nested objects
        cJSON *mode_json   = cJSON_GetObjectItem(item, "mode");
        cJSON *date_json   = cJSON_GetObjectItem(item, "ended_at");

        if (mid_json && cJSON_IsNumber(mid_json))
            records[i].match_id = mid_json->valueint;

        if (score_json && cJSON_IsNumber(score_json))
            records[i].final_score = score_json->valueint;

        if (winner_json)
            records[i].is_winner = cJSON_IsTrue(winner_json) ? 1 : 0;

        /* Map mode */
        records[i].mode = 0;
        if (mode_json && cJSON_IsString(mode_json)) {
            const char *m = mode_json->valuestring;
            // Support multiple variations of mode strings
            if (strcasecmp(m, "Scoring") == 0 || strcasecmp(m, "scoring") == 0)
                records[i].mode = 1;
            else if (strcasecmp(m, "Eliminated") == 0 || strcasecmp(m, "Elimination") == 0 || strcasecmp(m, "elimination") == 0)
                records[i].mode = 2;
        }

        /* Rank */
        if (rank_json && cJSON_IsNumber(rank_json)) {
            snprintf(
                records[i].ranking,
                sizeof(records[i].ranking),
                "%d",
                rank_json->valueint
            );
        } else {
            strcpy(records[i].ranking, "-");
        }

        /* Ended time */
        if (date_json && cJSON_IsString(date_json)) {
            records[i].ended_at = parse_iso8601(date_json->valuestring);
        }
    }

    *out_records = records;
    *out_count   = count;

    cJSON_Delete(response);
    return DB_SUCCESS;
}

db_error_t db_match_get_detail(
    uint32_t match_id,
    cJSON **out_json
) {
    if (match_id == 0 || !out_json) return DB_ERROR_INVALID_PARAM;
    
    // Complex SQL with multiple JOINs
    // Note: This is simplified - full implementation would need multiple queries or CTEs
    char query[2048];
    snprintf(query, sizeof(query), 
        "SELECT m.id, m.mode, m.max_players as player_count "
        "FROM matches m "
        "WHERE m.id = %u",
        match_id
    );
    
    // TODO: This complex query with nested joins (match_players, match_question, match_answer, profiles)
    // needs to be handled differently in PostgreSQL. Options:
    // 1. Use multiple queries and combine results in C code
    // 2. Create a PostgreSQL function to return the data in desired format
    // 3. Use PostgreSQL JSON aggregation functions
    // For now, keeping REST format as placeholder - needs proper implementation

    cJSON *res = NULL;
    db_error_t err = db_get("matches", query, &res);

    if (err != DB_SUCCESS) {
        if (res) cJSON_Delete(res);
        return err;
    }

    if (!cJSON_IsArray(res) || cJSON_GetArraySize(res) == 0) {
        cJSON_Delete(res);
        return DB_ERROR_NOT_FOUND;
    }

    // Detach the first match object
    cJSON *match_obj = cJSON_DetachItemFromArray(res, 0);
    cJSON_Delete(res);

    // TRANSFORM DATA
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "id", match_id);
    cJSON *mode = cJSON_GetObjectItem(match_obj, "mode");
    cJSON_AddStringToObject(root, "mode", mode ? mode->valuestring : "Unknown");
    
    cJSON *player_count = cJSON_GetObjectItem(match_obj, "player_count");
    cJSON_AddNumberToObject(root, "playerCount", player_count ? player_count->valueint : 0);

    // 1. Build Player Map (All participants)
    typedef struct {
        int id; // match_players.id
        int account_id;
        char name[64];
        bool eliminated;
        bool forfeited;
    } Participant;

    Participant players[10];
    int p_count = 0;

    cJSON *m_players = cJSON_GetObjectItem(match_obj, "match_players");
    if (cJSON_IsArray(m_players)) {
        p_count = cJSON_GetArraySize(m_players);
        if (p_count > 10) p_count = 10;
        
        for (int i = 0; i < p_count; i++) {
            cJSON *p = cJSON_GetArrayItem(m_players, i);
            players[i].id = cJSON_GetObjectItem(p, "id")->valueint;
            players[i].account_id = cJSON_GetObjectItem(p, "account_id")->valueint;
            players[i].eliminated = cJSON_IsTrue(cJSON_GetObjectItem(p, "eliminated"));
            players[i].forfeited = cJSON_IsTrue(cJSON_GetObjectItem(p, "forfeited"));
            
            // Get Name
            cJSON *acc = cJSON_GetObjectItem(p, "account");
            cJSON *prof_arr = acc ? cJSON_GetObjectItem(acc, "profile") : NULL;
            cJSON *prof = cJSON_IsArray(prof_arr) ? cJSON_GetArrayItem(prof_arr, 0) : prof_arr;
            cJSON *name = prof ? cJSON_GetObjectItem(prof, "name") : NULL;
            strncpy(players[i].name, (name && name->valuestring) ? name->valuestring : "Unknown", 63);
        }
    }

    // 2. Load Events
    typedef struct {
        int account_id;
        char type[32];
        int round;
        int q_idx;
    } MatchEvent;
    MatchEvent events[50];
    int e_count = 0;

    cJSON *m_events = cJSON_GetObjectItem(match_obj, "events");
    if (cJSON_IsArray(m_events)) {
        e_count = cJSON_GetArraySize(m_events);
        
        if (e_count > 50) e_count = 50;
        for (int i = 0; i < e_count; i++) {
            cJSON *ev = cJSON_GetArrayItem(m_events, i);
            events[i].account_id = cJSON_GetObjectItem(ev, "player_id")->valueint;
            strncpy(events[i].type, cJSON_GetObjectItem(ev, "event_type")->valuestring, 31);
            events[i].round = cJSON_GetObjectItem(ev, "round_no")->valueint;
            events[i].q_idx = cJSON_GetObjectItem(ev, "question_idx")->valueint;
        }
    }

    // 3. Process Questions
    cJSON *out_questions = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "questions", out_questions);

    cJSON *in_questions = cJSON_GetObjectItem(match_obj, "questions");
    if (cJSON_IsArray(in_questions)) {
        int q_count = cJSON_GetArraySize(in_questions);
        for (int i = 0; i < q_count; i++) {
            cJSON *q_in = cJSON_GetArrayItem(in_questions, i);
            cJSON *r_type = cJSON_GetObjectItem(q_in, "round_type");
            cJSON *q_json = cJSON_GetObjectItem(q_in, "question_text"); 
            int q_round = cJSON_GetObjectItem(q_in, "round_no")->valueint;
            int q_idx = cJSON_GetObjectItem(q_in, "question_idx")->valueint;

            // Extract question content
            char *q_text_str = "Question?";
            char *q_img_str = NULL;
            cJSON *choices_arr = NULL;
            if (q_json) {
                cJSON *txt = cJSON_GetObjectItem(q_json, "question");
                if (txt) q_text_str = txt->valuestring;
                cJSON *img = cJSON_GetObjectItem(q_json, "image");
                if (img) q_img_str = img->valuestring;
                choices_arr = cJSON_GetObjectItem(q_json, "choices");
            }

            cJSON *q_out = cJSON_CreateObject();
            cJSON_AddNumberToObject(q_out, "round_no", q_round);
            
            char *rt_str = r_type ? r_type->valuestring : "";
            if (r_type) cJSON_AddStringToObject(q_out, "round_type", rt_str);
            cJSON_AddNumberToObject(q_out, "question_idx", q_idx);

            if (strcasecmp(rt_str, "wheel") == 0) {
                // Wheel round has no question
                cJSON_AddStringToObject(q_out, "question_text", "");
                cJSON_AddNullToObject(q_out, "question_image");
            } else {
                cJSON_AddStringToObject(q_out, "question_text", q_text_str);
                if (q_img_str) cJSON_AddStringToObject(q_out, "question_image", q_img_str);
                else cJSON_AddNullToObject(q_out, "question_image");
            }

            // Process Answers for EVERY player
            cJSON *out_answers = cJSON_CreateArray();
            cJSON_AddItemToObject(q_out, "answers", out_answers);
            
            cJSON *in_answers = cJSON_GetObjectItem(q_in, "answers");

            for (int p_idx = 0; p_idx < p_count; p_idx++) {
                cJSON *a_out = cJSON_CreateObject();
                cJSON_AddStringToObject(a_out, "player_name", players[p_idx].name);
                
                // Check if player has an event status for this point in time
                const char *event_status = NULL;
                for (int e = 0; e < e_count; e++) {
                    if (events[e].account_id == players[p_idx].id) {
                        if (events[e].round < q_round || (events[e].round == q_round && events[e].q_idx <= q_idx)) {
                            // If event matches, use it. Convert event_type string to lowercase status if needed.
                            if (strcasecmp(events[e].type, "FORFEIT") == 0 || strcasecmp(events[e].type, "FORFEITED") == 0) {
                                event_status = "forfeit";
                            } else if (strcasecmp(events[e].type, "ELIMINATED") == 0) {
                                event_status = "eliminated";
                            }
                        }
                    }
                }

                if (event_status) {
                    cJSON_AddStringToObject(a_out, "status", event_status);
                    cJSON_AddBoolToObject(a_out, "is_correct", false);
                } else {
                    // Find if this player has an answer record
                    cJSON *found_ans = NULL;
                    if (cJSON_IsArray(in_answers)) {
                        for (int j = 0; j < cJSON_GetArraySize(in_answers); j++) {
                            cJSON *candidate = cJSON_GetArrayItem(in_answers, j);
                            if (cJSON_GetObjectItem(candidate, "player_id")->valueint == players[p_idx].id) {
                                found_ans = candidate;
                                break;
                            }
                        }
                    }

                    if (found_ans) {
                        cJSON *raw_ans = cJSON_GetObjectItem(found_ans, "answer");
                        cJSON *delta = cJSON_GetObjectItem(found_ans, "score_delta");
                        int score = delta ? delta->valueint : 0;
                        
                        cJSON *ans_val = raw_ans;
                        bool is_correct_from_db = (score > 0); // Fallback

                        if (cJSON_IsObject(raw_ans)) {
                            cJSON *internal = cJSON_GetObjectItem(raw_ans, "answer");
                            if (internal) ans_val = internal;

                            cJSON *correct_item = cJSON_GetObjectItem(raw_ans, "is_correct");
                            if (correct_item && (cJSON_IsBool(correct_item) || cJSON_IsNumber(correct_item))) {
                                is_correct_from_db = cJSON_IsTrue(correct_item) || (cJSON_IsNumber(correct_item) && correct_item->valueint != 0);
                            }
                        }
                        
                        char ans_display[128] = "-";
                        bool handled = false;
                        char *rt = r_type ? r_type->valuestring : "";

                        if (ans_val) {
                            if (strcasecmp(rt, "mcq") == 0 && cJSON_IsNumber(ans_val)) {
                                int idx = ans_val->valueint;
                                if (choices_arr && idx >= 0 && idx < cJSON_GetArraySize(choices_arr)) {
                                    cJSON *c = cJSON_GetArrayItem(choices_arr, idx);
                                    if (c && c->valuestring) {
                                        snprintf(ans_display, sizeof(ans_display), "%s", c->valuestring);
                                        handled = true;
                                    }
                                }
                            } else if (strcasecmp(rt, "bid") == 0) {
                                if (cJSON_IsNumber(ans_val)) {
                                    snprintf(ans_display, sizeof(ans_display), "$%d", ans_val->valueint);
                                    handled = true;
                                } else if (cJSON_IsString(ans_val)) {
                                    snprintf(ans_display, sizeof(ans_display), "$%s", ans_val->valuestring);
                                    handled = true;
                                }
                            } else if (strcasecmp(rt, "wheel") == 0 || strcasecmp(rt, "bonus") == 0) {
                                if (cJSON_IsObject(raw_ans)) {
                                    cJSON *t1 = cJSON_GetObjectItem(raw_ans, "turn1");
                                    cJSON *t2 = cJSON_GetObjectItem(raw_ans, "turn2");
                                    if (t1 && t2) {
                                        snprintf(ans_display, sizeof(ans_display), "%d + %d", t1->valueint, t2->valueint);
                                    } else if (t1) {
                                        snprintf(ans_display, sizeof(ans_display), "%d", t1->valueint);
                                    } else {
                                        if (cJSON_IsNumber(ans_val)) snprintf(ans_display, sizeof(ans_display), "%d", ans_val->valueint);
                                        else if (cJSON_IsString(ans_val)) snprintf(ans_display, sizeof(ans_display), "%s", ans_val->valuestring);
                                    }
                                } else {
                                    if (cJSON_IsNumber(ans_val)) snprintf(ans_display, sizeof(ans_display), "%d", ans_val->valueint);
                                    else if (cJSON_IsString(ans_val)) snprintf(ans_display, sizeof(ans_display), "%s", ans_val->valuestring);
                                }
                                handled = true;
                            }
                        }

                        if (!handled && ans_val) {
                            if (cJSON_IsString(ans_val)) snprintf(ans_display, sizeof(ans_display), "%s", ans_val->valuestring);
                            else if (cJSON_IsNumber(ans_val)) snprintf(ans_display, sizeof(ans_display), "%d", ans_val->valueint);
                        }

                        cJSON_AddStringToObject(a_out, "answer", ans_display);
                        cJSON_AddNumberToObject(a_out, "score_delta", score);
                        cJSON_AddBoolToObject(a_out, "is_correct", is_correct_from_db);
                    } else {
                        // Static check as fallback (unlikely if match_events works)
                        if (players[p_idx].forfeited) cJSON_AddStringToObject(a_out, "status", "forfeit");
                        else if (players[p_idx].eliminated) cJSON_AddStringToObject(a_out, "status", "eliminated");
                        cJSON_AddBoolToObject(a_out, "is_correct", false);
                    }
                }
                cJSON_AddItemToArray(out_answers, a_out);
            }
            cJSON_AddItemToArray(out_questions, q_out);
        }
    }
    
    cJSON_Delete(match_obj);
    *out_json = root;
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
        "{\\\"success\\\":true,\\\"message\\\":\\\"game started\\\"}"
    );

    return 0;
}

db_error_t db_match_event_insert(
    int64_t db_match_id,
    int32_t player_id,
    const char *event_type,
    int round_no,
    int question_idx
) {
    if (db_match_id <= 0 || !event_type) {
        printf("[MATCH_EVENT] Invalid parameters: db_match_id=%lld, event_type=%s\n",
               (long long)db_match_id, event_type ? event_type : "NULL");
        return DB_ERROR_INVALID_PARAM;
    }

    printf("[MATCH_EVENT] Inserting event: match_id=%lld, player_id=%d, type=%s, round=%d, q_idx=%d\n",
           (long long)db_match_id, player_id, event_type, round_no, question_idx);

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "match_id", db_match_id);
    cJSON_AddNumberToObject(payload, "player_id", player_id);
    cJSON_AddStringToObject(payload, "event_type", event_type);
    
    if (round_no > 0) {
        cJSON_AddNumberToObject(payload, "round_no", round_no);
    }
    if (question_idx >= 0) {
        cJSON_AddNumberToObject(payload, "question_idx", question_idx);
    }

    cJSON *response = NULL;
    db_error_t err = db_post("match_events", payload, &response);
    cJSON_Delete(payload);

    if (err != DB_SUCCESS) {
        printf("[MATCH_EVENT] Failed to insert event: err=%d\n", err);
        if (response) cJSON_Delete(response);
        return err;
    }

    if (response) cJSON_Delete(response);
    printf("[MATCH_EVENT] Event inserted successfully\n");
    return DB_SUCCESS;
}

db_error_t db_match_question_insert(
    int64_t db_match_id,
    int round_no,
    const char *round_type,
    int question_idx,
    const char *question_json,
    int64_t *out_question_id
) {
    if (db_match_id <= 0 || !round_type || !out_question_id) {
        printf("[MATCH_QUESTION] Invalid parameters\n");
        return DB_ERROR_INVALID_PARAM;
    }

    *out_question_id = 0;

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "match_id", db_match_id);
    cJSON_AddNumberToObject(payload, "round_no", round_no);
    cJSON_AddStringToObject(payload, "round_type", round_type);
    cJSON_AddNumberToObject(payload, "question_idx", question_idx);
    
    // Parse and add question JSON
    if (question_json) {
        cJSON *q_obj = cJSON_Parse(question_json);
        if (q_obj) {
            cJSON_AddItemToObject(payload, "question", q_obj);
        }
    }

    cJSON *response = NULL;
    db_error_t err = db_post("match_question", payload, &response);
    cJSON_Delete(payload);

    if (err != DB_SUCCESS) {
        printf("[MATCH_QUESTION] Failed to insert: err=%d\n", err);
        if (response) cJSON_Delete(response);
        return err;
    }

    // Extract ID from response
    if (cJSON_IsArray(response) && cJSON_GetArraySize(response) > 0) {
        cJSON *item = cJSON_GetArrayItem(response, 0);
        cJSON *id_json = cJSON_GetObjectItem(item, "id");
        if (id_json && cJSON_IsNumber(id_json)) {
            *out_question_id = id_json->valueint;
        }
    }

    if (response) cJSON_Delete(response);
    
    printf("[MATCH_QUESTION] Inserted: match=%lld round=%d idx=%d -> id=%lld\n",
           (long long)db_match_id, round_no, question_idx, (long long)*out_question_id);
    
    return DB_SUCCESS;
}

db_error_t db_match_answer_insert(
    int64_t question_id,
    int32_t player_id,
    const char *answer_json,
    int score_delta,
    int action_idx
) {
    if (question_id <= 0 || player_id <= 0) {
        printf("[MATCH_ANSWER] Invalid parameters: question_id=%lld, player_id=%d\n",
               (long long)question_id, player_id);
        return DB_ERROR_INVALID_PARAM;
    }

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "question_id", question_id);
    cJSON_AddNumberToObject(payload, "player_id", player_id);
    cJSON_AddNumberToObject(payload, "score_delta", score_delta);
    cJSON_AddNumberToObject(payload, "action_idx", action_idx > 0 ? action_idx : 1);
    
    // Parse and add answer JSON
    if (answer_json) {
        cJSON *ans_obj = cJSON_Parse(answer_json);
        if (ans_obj) {
            cJSON_AddItemToObject(payload, "answer", ans_obj);
        }
    }

    cJSON *response = NULL;
    db_error_t err = db_post("match_answer", payload, &response);
    cJSON_Delete(payload);

    if (err != DB_SUCCESS) {
        printf("[MATCH_ANSWER] Failed to insert: err=%d\n", err);
        if (response) cJSON_Delete(response);
        return err;
    }

    if (response) cJSON_Delete(response);
    
    printf("[MATCH_ANSWER] Inserted: question=%lld player=%d delta=%d\n",
           (long long)question_id, player_id, score_delta);
    
    return DB_SUCCESS;
}

db_error_t db_match_players_get_ids(
    int64_t db_match_id,
    int32_t *account_ids,
    int32_t *out_player_ids,
    int player_count
) {
    if (db_match_id <= 0 || !account_ids || !out_player_ids || player_count <= 0) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Initialize output to 0
    for (int i = 0; i < player_count; i++) {
        out_player_ids[i] = 0;
    }

    // Query match_players for this match
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT id, account_id FROM match_players WHERE match_id = %lld",
             (long long)db_match_id);

    cJSON *response = NULL;
    db_error_t err = db_get("match_players", query, &response);
    
    if (err != DB_SUCCESS) {
        printf("[MATCH_PLAYERS] Failed to get player IDs: err=%d\n", err);
        if (response) cJSON_Delete(response);
        return err;
    }

    if (!cJSON_IsArray(response)) {
        cJSON_Delete(response);
        return DB_ERROR_PARSE;
    }

    // Map account_id to match_player.id
    int count = cJSON_GetArraySize(response);
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(response, i);
        cJSON *id_json = cJSON_GetObjectItem(item, "id");
        cJSON *acc_json = cJSON_GetObjectItem(item, "account_id");
        
        if (!id_json || !acc_json) continue;
        
        int32_t mp_id = id_json->valueint;
        int32_t acc_id = acc_json->valueint;
        
        // Find matching account_id in input array
        for (int j = 0; j < player_count; j++) {
            if (account_ids[j] == acc_id) {
                out_player_ids[j] = mp_id;
                printf("[MATCH_PLAYERS] Mapped account %d -> match_player %d\n", acc_id, mp_id);
                break;
            }
        }
    }

    cJSON_Delete(response);
    return DB_SUCCESS;
}