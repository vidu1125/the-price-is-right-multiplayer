#include "handlers/match_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "handlers/session_manager.h"

// Storage for active matches
static MatchState g_matches[MAX_ACTIVE_MATCHES];
static int g_initialized = 0;

void match_manager_init(void) {
    memset(g_matches, 0, sizeof(g_matches));
    g_initialized = 1;
    printf("[HANDLER] <matchManager> Initialized (max capacity: %d)\n", MAX_ACTIVE_MATCHES);
}

/**
 * Find empty/available slot in match array
 */
static MatchState* alloc_slot(void) {
    for (int i = 0; i < MAX_ACTIVE_MATCHES; i++) {
        if (g_matches[i].runtime_match_id == 0) {
            return &g_matches[i];
        }
    }
    return NULL; // No free slots
}

MatchState* match_create(uint32_t match_id, uint32_t room_id) {
    if (!g_initialized) {
        printf("[HANDLER] <matchManager> ERROR: Not initialized\n");
        return NULL;
    }

    // Check if match already exists
    if (match_get_by_id(match_id) != NULL) {
        printf("[HANDLER] <matchManager> ERROR: Match ID %u already exists\n", match_id);
        return NULL;
    }

    MatchState *slot = alloc_slot();
    if (!slot) {
        printf("[HANDLER] <matchManager> ERROR: No free slots available\n");
        return NULL;
    }

    // Initialize match state
    memset(slot, 0, sizeof(MatchState));
    slot->runtime_match_id = match_id;
    slot->room_id = room_id;
    slot->db_match_id = 0; // Will be set when saved to DB
    slot->status = MATCH_WAITING; // Match is waiting to start
    slot->current_round_idx = 0;
    slot->created_at = time(NULL);
    slot->player_count = 0;
    slot->round_count = 0;

    printf("[HANDLER] <matchManager> Created match ID=%u (active: %d)\n", 
           match_id, match_get_count());

    return slot;
}

MatchState* match_get_by_id(uint32_t match_id) {
    if (match_id == 0) return NULL;
    
    for (int i = 0; i < MAX_ACTIVE_MATCHES; i++) {
        if (g_matches[i].runtime_match_id == match_id) {
            return &g_matches[i];
        }
    }
    return NULL;
}

MatchState* match_get_by_room(uint32_t room_id) {
    if (room_id == 0) return NULL;
    
    for (int i = 0; i < MAX_ACTIVE_MATCHES; i++) {
        if (g_matches[i].runtime_match_id != 0 && g_matches[i].room_id == room_id) {
            return &g_matches[i];
        }
    }
    return NULL;
}

void match_destroy(uint32_t match_id) {
    MatchState *match = match_get_by_id(match_id);
    if (!match) {
        printf("[HANDLER] <matchManager> WARN: Match ID %u not found for destroy\n", match_id);
        return;
    }

    // Free allocated json_data strings in all rounds
    for (int r = 0; r < match->round_count && r < MAX_MATCH_ROUNDS; r++) {
        RoundState *round = &match->rounds[r];
        for (int q = 0; q < round->question_count && q < MAX_QUESTIONS_PER_ROUND; q++) {
            if (round->question_data[q].json_data) {
                free(round->question_data[q].json_data);
            }
        }
    }

    printf("[HANDLER] <matchManager> Destroying match ID=%u\n", match_id);
    memset(match, 0, sizeof(MatchState));
}

int match_get_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_ACTIVE_MATCHES; i++) {
        if (g_matches[i].runtime_match_id != 0) {
            count++;
        }
    }
    return count;
}

void match_set_status(MatchState *match, MatchStatus status) {
    if (!match) return;
    
    match->status = status;
    printf("[HANDLER] <matchManager> Match ID=%u status updated to %d\n", 
           match->runtime_match_id, status);
}

void cleanup_disconnected_and_forfeit(uint32_t match_id) {
    MatchState *match = match_get_by_id(match_id);
    if (!match) return;

    for (int i = 0; i < match->player_count; i++) {
        MatchPlayerState *mp = &match->players[i];
        if (!mp || mp->eliminated) continue;

        // =====================================================
        // 1. FORFEIT: luôn eliminate, bất kể mode
        // =====================================================
        if (mp->forfeited) {
            mp->eliminated = 1;
            mp->eliminated_at_round = match->current_round_idx + 1;

            printf("[Cleanup] Player %d forfeited → eliminated (round %d)\n",
                   mp->account_id, mp->eliminated_at_round);

            // Optional nhưng rất nên có
            // notify + đưa về lobby
            UserSession *s = session_get_by_account(mp->account_id);
            if (s) {
                session_mark_lobby(s);
            }

            continue; // ⚠️ rất quan trọng
        }

        // =====================================================
        // 2. DISCONNECT
        // =====================================================
        UserSession *s = session_get_by_account(mp->account_id);
        bool connected = (s && s->state != SESSION_PLAYING_DISCONNECTED);

        if (!connected) {
            if (match->mode == MODE_ELIMINATION) {
                // elimination: disconnect -> eliminate
                mp->eliminated = 1;
                mp->eliminated_at_round = match->current_round_idx + 1;

                printf("[Cleanup] Player %d disconnected → eliminated (round %d)\n",
                       mp->account_id, mp->eliminated_at_round);

                if (s) {
                    session_mark_lobby(s);
                }
            } else {
                // scoring: KHÔNG eliminate
                printf("[Cleanup] Player %d disconnected (scoring mode) → 0 points this round\n",
                       mp->account_id);
                // Điểm 0 đã được đảm bảo bởi timeout / unanswered
            }
        }
    }
}

int count_active_players(uint32_t match_id) {
    MatchState *match = match_get_by_id(match_id);
    if (!match) return 0;

    int active = 0;
    for (int i = 0; i < match->player_count; i++) {
        MatchPlayerState *mp = &match->players[i];
        if (!mp) continue;
        if (mp->eliminated) continue;

        UserSession *s = session_get_by_account(mp->account_id);
        bool connected = (s && s->state != SESSION_PLAYING_DISCONNECTED);

        if (connected) active++;
    }
    return active;
}