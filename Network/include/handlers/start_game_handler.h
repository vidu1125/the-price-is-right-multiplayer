#ifndef START_GAME_HANDLER_H
#define START_GAME_HANDLER_H

#include <stdint.h>
#include <time.h>
#include "protocol/protocol.h"
#include "transport/room_manager.h"

#if defined(__GNUC__) || defined(__clang__)
  #define PACKED __attribute__((packed))
#else
  #pragma pack(push, 1)
  #define PACKED
#endif

// Payload for CMD_START_GAME
typedef struct PACKED {
    uint32_t room_id;
} StartGameRequest;

#if !defined(__GNUC__) && !defined(__clang__)
  #pragma pack(pop)
#endif

//==============================================================================
// Match State Definitions
//==============================================================================

typedef enum {
    PLAYING = 1,
    ENDED = 2
} MatchStatus;

typedef struct {
    int32_t account_id;
    int32_t score;
    uint8_t connected; // 1 = connected, 0 = disconnected
} MatchPlayerState;



#define MAX_MATCH_ROUNDS 10

typedef struct {
    uint32_t match_id;
    MatchStatus status;

    MatchPlayerState players[MAX_ROOM_MEMBERS]; 
    int player_count;

    // RoundState rounds[MAX_MATCH_ROUNDS];
    
    int current_round_idx;
    int current_question_idx;
    
    time_t deadline_ts;
    time_t created_at;
} MatchState;

void handle_start_game(
    int client_fd,
    MessageHeader *req,
    const char *payload
);

#endif // START_GAME_HANDLER_H
