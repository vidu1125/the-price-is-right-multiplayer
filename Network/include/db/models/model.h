#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

///////////////////////////////////////////////////////////////
// ACCOUNTS
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t id;
    char *email;
    char *password;
    char *role;           // default: "user"
    time_t created_at;
    time_t updated_at;
} account_t;

///////////////////////////////////////////////////////////////
// SESSIONS
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t account_id;    // PK
    char session_id[37];   // UUID
    bool connected;
    time_t updated_at;
} session_t;

///////////////////////////////////////////////////////////////
// PROFILES
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t id;
    int32_t account_id;

    char *name;
    char *avatar;
    char *bio;

    int32_t matches;
    int32_t wins;
    int32_t points;

    char *badges;          // jsonb (raw JSON)

    time_t created_at;
    time_t updated_at;
} profile_t;

///////////////////////////////////////////////////////////////
// FRIENDS
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t id;
    int32_t requester_id;
    int32_t addressee_id;

    char *status;          // pending | accepted | blocked

    time_t created_at;
    time_t updated_at;
} friend_t;

///////////////////////////////////////////////////////////////
// ROOMS
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t id;

    char *name;
    char *code;            // unique
    char *visibility;      // public | private

    int32_t host_id;
    char *status;          // waiting | playing | closed

    // Game settings
    char *mode;            // scoring | survival
    int32_t max_players;
    int32_t round_time;
    bool wager_mode;

    time_t created_at;
    time_t updated_at;
} room_t;

///////////////////////////////////////////////////////////////
// ROOM MEMBERS
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t room_id;
    int32_t account_id;

    bool ready;
    time_t joined_at;
} room_member_t;

///////////////////////////////////////////////////////////////
// MATCHES
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t id;
    int32_t room_id;

    char *mode;
    int32_t max_players;
    int32_t round_time;

    char *advanced;        // jsonb

    time_t started_at;
    time_t ended_at;
} match_t;

///////////////////////////////////////////////////////////////
// MATCH PLAYERS
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t id;
    int32_t match_id;
    int32_t account_id;

    int32_t score;
    bool eliminated;
    bool forfeited;
    bool winner;

    time_t joined_at;
} match_player_t;

///////////////////////////////////////////////////////////////
// MATCH QUESTIONS
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t id;
    int32_t match_id;

    int32_t round_no;
    char *round_type;      // MCQ | FILL | WHEEL | BONUS

    int32_t question_idx;
    char *question;        // jsonb snapshot

    time_t created_at;
} match_question_t;

///////////////////////////////////////////////////////////////
// MATCH ANSWERS
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t id;

    int32_t question_id;
    int32_t player_id;

    char *answer;          // jsonb
    int32_t score_delta;

    int32_t action_idx;
    time_t created_at;
} match_answer_t;

///////////////////////////////////////////////////////////////
// QUESTIONS
///////////////////////////////////////////////////////////////
typedef struct {
    int32_t id;
    char *type;            // MCQ | PRICE | WHEEL
    char *data;            // jsonb

    bool active;

    time_t created_at;
    time_t updated_at;
} question_t;
