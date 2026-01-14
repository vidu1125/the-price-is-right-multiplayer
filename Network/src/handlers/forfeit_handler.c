#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "handlers/forfeit_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "handlers/match_manager.h"
#include "handlers/session_manager.h"
#include "handlers/start_game_handler.h"
#include "db/repo/match_repo.h"

void handle_forfeit(int client_fd, MessageHeader *req, const char *payload) {
    if (req->length < sizeof(ForfeitRequest)) {
        printf("[HANDLER] <forfeit> Error: Invalid payload length %u from fd=%d\n", req->length, client_fd);
        return;
    }

    ForfeitRequest *request = (ForfeitRequest *)payload;
    uint32_t match_id = ntohl(request->match_id);

    printf("[HANDLER] <forfeit> Received forfeit request from fd=%d for match_id=%u\n", client_fd, match_id);

    // 1. Identify User
    UserSession *session = session_get_by_socket(client_fd);
    if (!session) {
        printf("[HANDLER] <forfeit> Error: No session found for fd=%d\n", client_fd);
        return;
    }
    int32_t account_id = session->account_id;

    // 2. Validate Match
    MatchState *match = match_get_by_id(match_id);
    if (!match) {
        printf("[HANDLER] <forfeit> Error: Match %u not found or not active\n", match_id);
        // Optional: Send error response
        return;
    }

    // 3. Find Player in Match
    MatchPlayerState *player = NULL;
    for (int i = 0; i < match->player_count; i++) {
        if (match->players[i].account_id == account_id) {
            player = &match->players[i];
            break;
        }
    }

    if (!player) {
         printf("[HANDLER] <forfeit> Error: Player %d not part of match %u\n", account_id, match_id);
         return;
    }

    // 4. Update Status
    // "Đổi trạng thái forfeit và eliminiated thành true"
    player->forfeited = 1;
    player->eliminated = 1;

    // Get current context for logging
    int r_idx = match->current_round_idx;
    int q_idx = 0;
    if (r_idx >= 0 && r_idx < match->round_count) {
        q_idx = match->rounds[r_idx].current_question_idx;
    }

    printf("[HANDLER] <forfeit> Player %d forfeited match %u at Round %d, Question %d. Status updated.\n", 
           account_id, match_id, r_idx + 1, q_idx + 1);

    // 6. Persist Event to Database
    db_error_t db_err = db_match_event_insert(
        match->db_match_id,
        account_id,
        "FORFEIT",
        r_idx + 1,  // Round 1-indexed
        q_idx       // Question 0-indexed (as stored)
    );

    if (db_err != DB_SUCCESS) {
        printf("[FORFEIT] Warning: Failed to persist event to DB (err=%d). Continuing...\n", db_err);
        // Don't fail the forfeit if DB insert fails - memory state is already updated
    }

    // 7. Send Success Response
    // Empty JSON success
    const char *resp = "{}"; 
    forward_response(client_fd, req, RES_SUCCESS, resp, 2);
}
