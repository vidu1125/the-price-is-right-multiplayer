#ifndef START_GAME_HANDLER_H
#define START_GAME_HANDLER_H

#include <stdint.h>
#include <time.h>
#include "protocol/protocol.h"
#include "transport/room_manager.h"

#define MAX_MATCH_PLAYERS 8
#define MAX_MATCH_ROUNDS 6
#define MAX_QUESTIONS_PER_ROUND 10

typedef struct PACKED {
    uint32_t room_id;
} StartGameRequest;


//==============================================================================
// Match State Definitions
//==============================================================================

typedef enum {
    MATCH_WAITING = 0,
    MATCH_PLAYING = 1,
    MATCH_ENDED   = 2
} MatchStatus;


typedef enum {
    ROUND_MCQ   = 1,   // round 1
    ROUND_BID   = 2,   // round 2
    ROUND_WHEEL = 3,   // round 3
    ROUND_BONUS = 4    // optional
} RoundType;

typedef enum {
    ROUND_PENDING = 0,
    ROUND_PLAYING = 1,
    ROUND_ENDED   = 2
} RoundStatus;

typedef enum {
    QUESTION_PENDING = 0,
    QUESTION_ACTIVE  = 1,
    QUESTION_ENDED   = 2
} QuestionStatus;


// ============================================================================
// STRUCT DEFINITIONS (ordered by dependencies)
// ============================================================================

// Question data (no dependencies)
typedef struct {
    int round;           // Round number (1-based)
    int index;           // Question index within round (0-based)
    char *json_data;     // JSON string containing question data
} MatchQuestion;

// Question state (no dependencies)
typedef struct {
    int32_t question_id;     // DB id
    QuestionStatus status;

    int answered_count;
    int correct_count;
} QuestionState;

// Player state (no dependencies)
typedef struct {
    int32_t account_id;
    int32_t match_player_id; // map DB nếu cần

    int32_t score;           // điểm hiện tại (authoritative)
    int32_t score_delta;     // điểm cộng/trừ ở câu gần nhất

    uint8_t connected;       // 1 = online, 0 = disconnected
    uint8_t eliminated;      // 1 = bị loại
    uint8_t forfeited;       // 1 = bỏ cuộc
} MatchPlayerState;

// Round state (depends on QuestionState, MatchQuestion)
typedef struct {
    int index;                  // Vị trí của round trong match (0-based)

    RoundType type;              // MCQ | BID | WHEEL | BONUS
    RoundStatus status;          // PENDING | PLAYING | ENDED

    QuestionState questions[MAX_QUESTIONS_PER_ROUND];
    MatchQuestion question_data[MAX_QUESTIONS_PER_ROUND];

    int question_count;          // Số câu thực sự của round
    int current_question_idx;    // Câu hiện tại đang chơi (0-based)

    time_t started_at;           // Thời điểm round bắt đầu
    time_t ended_at;             // Thời điểm round kết thúc (0 nếu chưa)
} RoundState;

// Match state (depends on MatchPlayerState, RoundState)
typedef struct {
    uint32_t runtime_match_id;
    int64_t  db_match_id;

    MatchStatus status;
    GameMode mode;
    MatchPlayerState players[MAX_MATCH_PLAYERS];
    int player_count;

    RoundState rounds[MAX_MATCH_ROUNDS];
    int round_count;

    int current_round_idx;

    time_t created_at;
} MatchState;


void handle_start_game(
    int client_fd,
    MessageHeader *req,
    const char *payload
);

#endif // START_GAME_HANDLER_H
