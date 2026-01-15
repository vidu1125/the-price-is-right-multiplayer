#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "handlers/forfeit_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "handlers/match_manager.h"
#include "handlers/session_manager.h"
#include "handlers/start_game_handler.h"
#include "transport/room_manager.h"
#include "db/repo/match_repo.h"
#include <cjson/cJSON.h>
#include <stdlib.h>

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

    // 5. Send NTF_ELIMINATION to the forfeiting player (so UI can redirect to lobby)
    cJSON *ntf = cJSON_CreateObject();
    cJSON_AddNumberToObject(ntf, "player_id", account_id);
    cJSON_AddNumberToObject(ntf, "room_id", match->room_id);
    cJSON_AddStringToObject(ntf, "reason", "FORFEIT");
    cJSON_AddNumberToObject(ntf, "round", r_idx + 1);
    cJSON_AddNumberToObject(ntf, "final_score", player->score);
    cJSON_AddStringToObject(ntf, "message", "You have forfeited the match!");
    
    char *ntf_json = cJSON_PrintUnformatted(ntf);
    cJSON_Delete(ntf);
    
    if (ntf_json) {
        MessageHeader hdr = {0};
        hdr.magic = htons(MAGIC_NUMBER);
        hdr.version = PROTOCOL_VERSION;
        hdr.command = htons(NTF_ELIMINATION);
        hdr.length = htonl((uint32_t)strlen(ntf_json));
        
        send(client_fd, &hdr, sizeof(hdr), 0);
        send(client_fd, ntf_json, strlen(ntf_json), 0);
        printf("[FORFEIT] Sent NTF_ELIMINATION to player %d\n", account_id);
        free(ntf_json);
    }

    // 6. Change session state to LOBBY so player can join new games
    session_mark_lobby(session);
    printf("[FORFEIT] Changed player %d session state to LOBBY (Staying in room)\n", account_id);

    // 7. Reset player readiness in the room but stay in room
    uint32_t room_id = room_find_by_player_fd(client_fd);
    if (room_id > 0) {
        room_set_ready(room_id, client_fd, false);
        broadcast_player_list(room_id);
    }
    printf("[FORFEIT] Reset player %d readiness in room %u\n", account_id, room_id);

    // 8. Persist Event to Database
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

    // 9. Send Success Response
    const char *resp = "{\"success\":true}"; 
    forward_response(client_fd, req, RES_SUCCESS, resp, (uint32_t)strlen(resp));
}
