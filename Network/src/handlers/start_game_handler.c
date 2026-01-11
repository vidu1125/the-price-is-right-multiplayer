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
    // VALIDATION: Check room state, host, and readiness
    // =========================================================================
    // TODO: Validate room state
    // - Check if room exists in database
    // - Check if requester is host (via session_get_by_socket)
    // - Check if room is full/ready
    // - Check if room is not already playing
    
    UserSession *session = session_get_by_socket(client_fd);
    if (!session || session->state != SESSION_LOBBY) {
        printf("[HANDLER] <startgame> Invalid session state\n");
        return;
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

    printf("[HANDLER] <startgame> Step 1: Match created via manager (ID: %u)\n", 
           match->runtime_match_id);

    // Insert match into database
    int64_t db_match_id = 0;
    db_error_t db_rc = db_match_insert(room_id, "classic", 4, &db_match_id);
    if (db_rc != DB_OK) {
        printf("[HANDLER] <startgame> WARN: Failed to insert match into DB (rc=%d), continuing...\n", db_rc);
        // Continue anyway - match exists in memory
    } else {
        match->db_match_id = db_match_id;
        printf("[HANDLER] <startgame> Match saved to database (db_match_id=%lld)\n", (long long)db_match_id);
    }

    // =========================================================================
    // STEP 2: ADD PLAYERS to MatchState
    // =========================================================================
    // TODO: Get all room members from room_manager
    int test_accounts[] = {45, 40, 44, 48};
    match->player_count = 4;
    
    for (int i = 0; i < 4; i++) {
        match->players[i].account_id = test_accounts[i];
        match->players[i].score = 0;
        match->players[i].connected = 1;
    }
    
    printf("[HANDLER] <startgame> Step 2: Added %d players to match\n", match->player_count);

    // Insert match_players into database
    if (match->db_match_id > 0) {
        db_error_t player_rc = db_match_players_insert(match->db_match_id, test_accounts, 4);
        if (player_rc != DB_OK) {
            printf("[HANDLER] <startgame> WARN: Failed to insert match_players (rc=%d)\n", player_rc);
        } else {
            printf("[HANDLER] <startgame> Match players saved to database\n");
        }
    }
    
    // =========================================================================
    // STEP 3: CHUYỂN USER SESSION → SESSION_PLAYING
    // =========================================================================
    // Update all player sessions to PLAYING state
    printf("[HANDLER] <startgame> Step 3: Updating player sessions to PLAYING...\n");
    
    int sessions_updated = 0;
    for (int i = 0; i < match->player_count; i++) {
        int32_t account_id = match->players[i].account_id;
        
        // Get session by account_id
        UserSession *session = session_get_by_account(account_id);
        if (!session) {
            printf("[HANDLER] <startgame> WARN: Session not found for account_id=%d\n", account_id);
            continue;
        }
        
        // Mark session as PLAYING
        session_mark_playing(session);
        sessions_updated++;
        
        printf("[HANDLER] <startgame>   - Player account_id=%d → SESSION_PLAYING\n", account_id);
    }

    printf("[HANDLER] <startgame> Step 3: Updated %d/%d player sessions to PLAYING\n", 
           sessions_updated, match->player_count);

    // =========================================================================
    // STEP 4: CREATE 3 ROUNDS AND LOAD QUESTIONS
    // =========================================================================
    printf("[HANDLER] <startgame> Step 4: Creating rounds and loading questions...\n");
    
    // Define round structure: MCQ, BID, WHEEL
    RoundType round_types[] = {ROUND_MCQ, ROUND_BID, ROUND_WHEEL};
    const char *round_type_names[] = {"mcq", "bid", "wheel"};
    int questions_per_round[] = {5, 3, 2}; // MCQ=5, BID=3, WHEEL=2
    
    match->round_count = 3;
    int total_questions_loaded = 0;
    
    for (int r = 0; r < 3; r++) {
        RoundState *round = &match->rounds[r];
        
        // Initialize round
        memset(round, 0, sizeof(RoundState));
        round->index = r;
        round->type = round_types[r];
        round->status = ROUND_PENDING;
        round->question_count = 0;
        round->current_question_idx = 0;
        round->started_at = 0;
        round->ended_at = 0;
        
        // Load questions for this round filtered by round_type
        cJSON *questions_json = NULL;
        db_error_t db_rc = question_get_random(round_type_names[r], questions_per_round[r], &questions_json);
        
        if (db_rc != DB_OK || !questions_json) {
            printf("[HANDLER] <startgame> ERROR: Failed to load questions for round %d (rc=%d)\n", 
                   r + 1, db_rc);
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
    
    match->current_round_idx = 0;
    
    // =========================================================================
    // MATCH CREATION SUMMARY
    // =========================================================================
    printf("\n");
    printf("[HANDLER] <startgame> ========================================\n");
    printf("[HANDLER] <startgame> MATCH CREATED SUCCESSFULLY\n");
    printf("[HANDLER] <startgame> ========================================\n");
    printf("[HANDLER] <startgame> Runtime Match ID: %u\n", match->runtime_match_id);
    printf("[HANDLER] <startgame> Database Match ID: %lld (not saved yet)\n", (long long)match->db_match_id);
    printf("[HANDLER] <startgame> Total Rounds: %d\n", match->round_count);
    printf("[HANDLER] <startgame> Total Questions Loaded: %d\n", total_questions_loaded);
    printf("[HANDLER] <startgame>   - Round 1 (MCQ): %d questions\n", match->rounds[0].question_count);
    printf("[HANDLER] <startgame>   - Round 2 (BID): %d questions\n", match->rounds[1].question_count);
    printf("[HANDLER] <startgame>   - Round 3 (WHEEL): %d questions\n", match->rounds[2].question_count);
    printf("[HANDLER] <startgame> Match Status: WAITING\n");
    printf("[HANDLER] <startgame> ========================================\n");
    printf("\n");

    // =========================================================================
    // STEP 5: BROADCAST START ROUND 1
    // =========================================================================
    // TODO: Create round start notification payload
    // - Include round number, questions, timer, etc.
    // - Use room_broadcast to send to all players
    
    // Example broadcast structure (needs actual payload implementation)
    const char *round_start_msg = "{\"round\":1,\"status\":\"started\"}";
    room_broadcast(
        room_id,
        0x0501, // TODO: Define NTF_ROUND_START in opcode.h
        round_start_msg,
        strlen(round_start_msg),
        -1 // Send to all members
    );

    printf("[HANDLER] <startgame> Step 6: Round 1 start broadcasted to room %u\n", room_id);
    printf("[HANDLER] <startgame> Game start sequence completed successfully\n");
}
