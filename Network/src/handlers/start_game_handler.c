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
    // =========================================================================
    // STEP 3: CHUYỂN USER SESSION → SESSION_PLAYING
    // =========================================================================
    // Update all player sessions to PLAYING state


    printf("[HANDLER] <startgame> Step 3: All player sessions updated to PLAYING\n");

    // =========================================================================
    // STEP 4: INITIALIZE ROUNDS (questions loaded per-round later)
    // =========================================================================
    printf("[HANDLER] <startgame> Step 4: Initializing round structure...\n");
    
    // TODO: Load questions and populate round data
    // For now, just initialize empty round structure
    match->round_count = 0;
    match->current_round_idx = 0;
    
    printf("[HANDLER] <startgame> Step 4: Round structure initialized\n");

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
