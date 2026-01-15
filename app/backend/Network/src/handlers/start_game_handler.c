#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include "handlers/start_game_handler.h"
#include "handlers/session_manager.h"
#include "handlers/match_manager.h"
#include "transport/socket_server.h"
#include "transport/room_manager.h"
#include "protocol/opcode.h"
#include "db/repo/match_repo.h"
#include "db/repo/question_repo.h"

void handle_start_game(int client_fd, MessageHeader *req, const char *payload) {
    if (req->length != sizeof(StartGameRequest)) {
        printf("[HANDLER] <startgame> Invalid payload length: %u\n", req->length);
        return;
    }

    StartGameRequest *request = (StartGameRequest *)payload;
    uint32_t room_id = ntohl(request->room_id);

    printf("[HANDLER] <startgame> Request received for Room ID: %u\n", room_id);

    // =========================================================================
    // VALIDATION: Check session state
    // =========================================================================
    UserSession *session = session_get_by_socket(client_fd);
    if (!session) {
        printf("[HANDLER] <startgame> Error: No session found for client_fd=%d\n", client_fd);
        forward_response(client_fd, req, ERR_BAD_REQUEST, "No session found", 16);
        return;
    }

    printf("[HANDLER] <startgame> Requester Account ID: %d, current state: %d\n", 
           session->account_id, session->state);

    if (session->state != SESSION_LOBBY) {
        // SPECIAL CASE: If session is PLAYING, allow reset to LOBBY if they are initiating a new game start
        // This handles cases where a previous match didn't cleanup correctly.
        if (session->state == SESSION_PLAYING) {
            printf("[HANDLER] <startgame> INFO: Requester is in SESSION_PLAYING state. Resetting to LOBBY.\n");
            session->state = SESSION_LOBBY;
        } else {
            printf("[HANDLER] <startgame> Error: Invalid session state (%d)\n", session->state);
            forward_response(client_fd, req, ERR_BAD_REQUEST, "Invalid session state", 21);
            return;
        }
    }

    // =========================================================================
    // VALIDATION: Check room exists and get state
    // =========================================================================
    RoomState *room = room_get(room_id);
    if (!room) {
        printf("[HANDLER] <startgame> Error: Room %u not found\n", room_id);
        forward_response(client_fd, req, ERR_BAD_REQUEST, "Room not found", 14);
        return;
    }

    // =========================================================================
    // VALIDATION: Check if requester is host
    // =========================================================================
    if (room->host_id != session->account_id) {
        printf("[HANDLER] <startgame> Error: Only host can start game (host=%u, requester=%d)\n", 
               room->host_id, session->account_id);
        forward_response(client_fd, req, ERR_BAD_REQUEST, "Only host can start game", 24);
        return;
    }

    // =========================================================================
    // VALIDATION: Check room status is WAITING
    // =========================================================================
    if (room->status != ROOM_WAITING) {
        printf("[HANDLER] <startgame> Error: Room status is not WAITING (status=%d)\n", room->status);
        forward_response(client_fd, req, ERR_BAD_REQUEST, "Room not in waiting state", 26);
        return;
    }
    for (int i = 0; i < room->player_count; i++) {
    RoomPlayerState *p = &room->players[i];

    if (!p->connected) {
        printf("[HANDLER] <startgame> Error: Player %u disconnected\n", p->account_id);
        forward_response(
            client_fd,
            req,
            ERR_BAD_REQUEST,
            "Player disconnected",
            19
        );
        return;
    }

    if (!p->is_ready) {
        printf("[HANDLER] <startgame> Error: Player %u not ready\n", p->account_id);
        forward_response(
            client_fd,
            req,
            ERR_BAD_REQUEST,
            "Not all players are ready",
            26
        );
        return;
    }
}
    
    printf("[HANDLER] <startgame> All players are ready\n");

    // =========================================================================
    // VALIDATION: Check player count based on game mode
    // =========================================================================
    uint8_t player_count = room->player_count;
    
    printf("[HANDLER] <startgame> Starting with %d player(s)\n", player_count);
    
    if (player_count < 1) {
        forward_response(client_fd, req, ERR_BAD_REQUEST, "Need at least 2 players to start", 32);
        return;
    }

    // =========================================================================
    // VALIDATION: Check all players are connected
    // =========================================================================
    for (uint8_t i = 0; i < player_count; i++) {
        if (!room->players[i].connected) {
            printf("[HANDLER] <startgame> Error: Player account_id=%u is not connected\n", 
                   room->players[i].account_id);
            forward_response(client_fd, req, ERR_BAD_REQUEST, 
                           "All players must be connected", 30);
            return;
        }
    }

    // =========================================================================
    // STEP 1: CREATE MATCH REALTIME (In-memory MatchState)
    // =========================================================================
    // Generate temporary match ID
    time_t now = time(NULL);
    uint32_t match_id = (uint32_t)(now * 1000 + room_id);
    
    // Create match via match manager
    MatchState *match = match_create(match_id, room_id);
    if (!match) {
        printf("[HANDLER] <startgame> Failed to create match\n");
        return;
    }

    // ⭐ IMPORTANT: Copy game mode from room to match
    match->mode = room->mode;

    printf("[HANDLER] <startgame> Step 1: Match created via manager (ID: %u, mode=%s)\n", 
           match->runtime_match_id, room->mode == MODE_ELIMINATION ? "elimination" : "scoring");

    // Insert match into database
    int64_t db_match_id = 0;
    const char *mode_str = (room->mode == MODE_ELIMINATION) ? "elimination" : "scoring";
    
    db_error_t db_rc = db_match_insert(room_id, mode_str, player_count, &db_match_id);
    if (db_rc != DB_OK) {
        printf("[HANDLER] <startgame> WARN: Failed to insert match into DB (rc=%d), continuing...\n", db_rc);
        // Continue anyway - match exists in memory
    } else {
        match->db_match_id = db_match_id;
        printf("[HANDLER] <startgame> Match saved to database (db_match_id=%lld, mode=%s)\n", 
               (long long)db_match_id, mode_str);
    }

    // =========================================================================
    // STEP 2: ADD PLAYERS to MatchState
    // =========================================================================
    match->player_count = room->player_count; // Use actual room player count
    
    for (int i = 0; i < match->player_count; i++) {
        match->players[i].account_id = room->players[i].account_id;
        match->players[i].score = 0;
        match->players[i].connected = room->players[i].connected ? 1 : 0;
        
        // Copy player name
        strncpy(match->players[i].name, room->players[i].name, sizeof(match->players[i].name) - 1);
        match->players[i].name[sizeof(match->players[i].name) - 1] = '\0';
        
        printf("[HANDLER] <startgame>   - Added match player: %d\n", 
               match->players[i].account_id);
    }
    
    printf("[HANDLER] <startgame> Step 2: Added %d players to match from Room %u\n", 
           match->player_count, room_id);

    // Insert match_players into database
    if (match->db_match_id > 0) {
        // Collect account IDs into array for DB call
        int32_t player_ids[MAX_ROOM_MEMBERS];
        for(int i=0; i<match->player_count; i++) {
            player_ids[i] = match->players[i].account_id;
        }
    
        db_error_t player_rc = db_match_players_insert(match->db_match_id, player_ids, match->player_count);
        if (player_rc != DB_OK) {
            printf("[HANDLER] <startgame> WARN: Failed to insert match_players (rc=%d)\n", player_rc);
        } else {
            printf("[HANDLER] <startgame> Match players saved to database\n");
            
            // Get match_player IDs from database and store in MatchPlayerState
            int32_t match_player_ids[MAX_ROOM_MEMBERS];
            db_error_t ids_rc = db_match_players_get_ids(
                match->db_match_id,
                player_ids,
                match_player_ids,
                match->player_count
            );
            
            if (ids_rc == DB_OK) {
                for (int i = 0; i < match->player_count; i++) {
                    match->players[i].match_player_id = match_player_ids[i];
                    printf("[HANDLER] <startgame>   - Player %d -> match_player_id=%d\n",
                           match->players[i].account_id, match->players[i].match_player_id);
                }
            } else {
                printf("[HANDLER] <startgame> WARN: Failed to get match_player IDs (rc=%d)\n", ids_rc);
            }
        }
    }
    
    printf("[HANDLER] <startgame> Step 3: Deferred session update (will update after questions are loaded)\n");

    // =========================================================================
    // STEP 4: GET EXCLUDED QUESTION IDs (avoid duplicates from recent matches)
    // =========================================================================
    printf("[HANDLER] <startgame> Step 4: Getting excluded question IDs from recent matches...\n");
    
    // Collect player account IDs
    int32_t player_account_ids[MAX_ROOM_MEMBERS];
    for (int i = 0; i < match->player_count; i++) {
        player_account_ids[i] = match->players[i].account_id;
    }

    // Get excluded question IDs from recent matches (last 3 matches)
    int32_t *excluded_ids = NULL;
    int excluded_count = 0;

    db_error_t excl_rc = question_get_excluded_ids(
        player_account_ids,
        match->player_count,
        3,  // recent_match_count: check last 3 matches
        &excluded_ids,
        &excluded_count
    );

    if (excl_rc != DB_OK) {
        printf("[HANDLER] <startgame> WARN: Failed to get excluded IDs (rc=%d), continuing without exclusions\n", excl_rc);
        excluded_ids = NULL;
        excluded_count = 0;
    } else {
        printf("[HANDLER] <startgame> Excluding %d questions from recent matches\n", excluded_count);
    }

    // =========================================================================
    // STEP 5: CREATE 3 ROUNDS AND LOAD QUESTIONS
    // =========================================================================
    printf("[HANDLER] <startgame> Step 5: Creating rounds and loading questions...\n");
    
    RoundType round_types[] = {ROUND_MCQ, ROUND_BID, ROUND_WHEEL};
    const char *round_type_names[] = {"mcq", "bid", "wheel"};
    // Questions per round (MCQ=5, BID=5, WHEEL=1)
    int questions_per_round[] = {5, 5, 1};
    
    match->round_count = 3;
    int total_questions_loaded = 0;
    
    for (int r = 0; r < 3; r++) {
        RoundState *round = &match->rounds[r];
        
        memset(round, 0, sizeof(RoundState));
        round->index = r;
        round->type = round_types[r];
        round->status = ROUND_PENDING;
        round->question_count = 0;
        round->current_question_idx = 0;
        round->started_at = 0;
        round->ended_at = 0;
        
        // ⭐ SKIP question loading for ROUND_WHEEL (Round 3)
        if (round->type == ROUND_WHEEL) {
            printf("[HANDLER] <startgame> Round %d (WHEEL): Skipping question loading\n", r + 1);
            continue;
        }

        cJSON *questions_json = NULL;
        db_error_t db_rc = question_get_random(
            round_type_names[r],      // round_type
            questions_per_round[r],   // count
            excluded_ids,             // excluded_ids (may be NULL)
            excluded_count,           // excluded_count
            NULL,                     // category (NULL = all categories)
            &questions_json           // out_json
        );
        
        if (db_rc != DB_OK || !questions_json) {
            printf("[HANDLER] <startgame> ERROR: Failed to load questions for round %d (rc=%d)\n", 
                   r + 1, db_rc);
            if (excluded_ids) free(excluded_ids);
            match_destroy(match->runtime_match_id);
            return;
        }
        
        // Populate questions for this round
        int fetched_count = cJSON_GetArraySize(questions_json);
        for (int q = 0; q < fetched_count && q < MAX_QUESTIONS_PER_ROUND; q++) {
            cJSON *question_obj = cJSON_GetArrayItem(questions_json, q);
            if (!question_obj) continue;
            
            // Store question data
            MatchQuestion *mq = &round->question_data[q];
            mq->round = r + 1;
            mq->index = q;
            
            // Normalize 'content' to 'question' for consistency
            cJSON *content_item = cJSON_GetObjectItem(question_obj, "content");
            if (content_item) {
                cJSON_AddItemToObject(question_obj, "question", cJSON_Duplicate(content_item, 1));
                cJSON_DeleteItemFromObject(question_obj, "content");
            }

            mq->json_data = cJSON_PrintUnformatted(question_obj);
            
            // Initialize question state
            QuestionState *qs = &round->questions[q];
            cJSON *id_field = cJSON_GetObjectItem(question_obj, "id");
            qs->question_id = id_field ? id_field->valueint : 0;
            qs->status = QUESTION_PENDING;
            qs->answered_count = 0;
            qs->correct_count = 0;
            
            round->question_count++;
            total_questions_loaded++;
        }
        
        cJSON_Delete(questions_json);
        
        printf("[HANDLER] <startgame> Round %d (%s): Loaded %d questions\n", 
               r + 1, 
               round->type == ROUND_MCQ ? "MCQ" : round->type == ROUND_BID ? "BID" : "WHEEL",
               round->question_count);
    }
    
    // Free excluded_ids after loading all questions
    if (excluded_ids) {
        free(excluded_ids);
    }
    
    match->current_round_idx = 0;

    // =========================================================================
    // STEP 6: INSERT MATCH_QUESTIONS INTO DATABASE
    // =========================================================================
    if (match->db_match_id > 0) {
        printf("[HANDLER] <startgame> Step 6: Inserting match_questions into database...\n");
        
        int questions_inserted = 0;
        for (int r = 0; r < match->round_count; r++) {
            RoundState *round = &match->rounds[r];
            const char *round_type_str = (round->type == ROUND_MCQ) ? "MCQ" :
                                         (round->type == ROUND_BID) ? "BID" :
                                         (round->type == ROUND_WHEEL) ? "WHEEL" : "UNKNOWN";
            
            for (int q = 0; q < round->question_count; q++) {
                int64_t mq_id = 0;
                db_error_t mq_rc = db_match_question_insert(
                    match->db_match_id,
                    r + 1,  // round_no (1-based)
                    round_type_str,
                    q,      // question_idx (0-based)
                    round->question_data[q].json_data,
                    &mq_id
                );
                
                if (mq_rc == DB_OK && mq_id > 0) {
                    // Store match_question.id in QuestionState for later use
                    round->questions[q].question_id = (int32_t)mq_id;
                    questions_inserted++;
                } else {
                    printf("[HANDLER] <startgame> WARN: Failed to insert match_question r=%d q=%d\n", r+1, q);
                }
            }
        }
        
        printf("[HANDLER] <startgame> Inserted %d match_questions into database\n", questions_inserted);
    }

    // =========================================================================
    // MATCH CREATION SUMMARY
    // =========================================================================
    printf("\n");
    printf("[HANDLER] <startgame> ========================================\n");
    printf("[HANDLER] <startgame> MATCH CREATED SUCCESSFULLY\n");
    printf("[HANDLER] <startgame> ========================================\n");
    printf("[HANDLER] <startgame> Runtime Match ID: %u\n", (unsigned int)match->runtime_match_id);
    printf("[HANDLER] <startgame> Database Match ID: %lld (not saved yet)\n", (long long)match->db_match_id);
    printf("[HANDLER] <startgame> Total Rounds: %d\n", match->round_count);
    printf("[HANDLER] <startgame> Total Questions Loaded: %d\n", total_questions_loaded);
    printf("[HANDLER] <startgame>   - Round 1 (MCQ): %d questions\n", match->rounds[0].question_count);
    printf("[HANDLER] <startgame>   - Round 2 (BID): %d questions\n", match->rounds[1].question_count);
    printf("[HANDLER] <startgame>   - Round 3 (WHEEL): No questions (Wheel Spin)\n");
    printf("[HANDLER] <startgame> Match Status: WAITING\n");
    printf("[HANDLER] <startgame> ========================================\n");
    printf("\n");
    printf("\n");

    // =========================================================================
    // STEP 7: CHUYỂN USER SESSION → SESSION_PLAYING
    // =========================================================================
    printf("[HANDLER] <startgame> Step 7: Finalizing session states...\n");
    for (int i = 0; i < match->player_count; i++) {
        UserSession *s = session_get_by_account(match->players[i].account_id);
        if (s) {
            session_mark_playing(s);
            printf("[HANDLER] <startgame>   - Player %d → SESSION_PLAYING\n", s->account_id);
        }
    }

    // =========================================================================
    // STEP 8: BROADCAST NTF_GAME_START TO ALL PLAYERS
    // =========================================================================
    // This triggers the client to navigate to the game screen (Round 1)
    char game_start_msg[256];
    snprintf(game_start_msg, sizeof(game_start_msg), "{\"match_id\":%u}", match->runtime_match_id);

    printf("[HANDLER] <startgame> Broadcasting NTF_GAME_START (match_id=%u) to room %u\n", 
           match->runtime_match_id, room_id);

    room_broadcast(
        room_id,
        NTF_GAME_START, // 0x02C4 - Triggers navigateToRound1() in frontend
        game_start_msg,
        strlen(game_start_msg),
        -1 // Send to ALL members including host
    );

    printf("[HANDLER] <startgame> Game start broadcast complete. All clients should navigate now.\n");

    printf("[HANDLER] <startgame> Game start sequence completed successfully\n");
}
