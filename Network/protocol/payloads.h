#ifndef PAYLOADS_H
#define PAYLOADS_H

#include <stdint.h>
#include "packet_format.h"

// ===== TIME SYNC =====
typedef struct PACKED {
    uint64_t client_send_time;
} TimeSyncRequest;

typedef struct PACKED {
    uint64_t client_send_time;
    uint64_t server_recv_time;
    uint64_t server_send_time;
} TimeSyncResponse;

// ===== CREATE ROOM =====
typedef struct PACKED {
    char room_name[64];
    uint8_t visibility;        // 0: public, 1: private
    uint8_t mode;              // 0: scoring, 1: elimination
    uint8_t max_players;       // 4-6
    uint16_t round_time;       // 15-60 seconds
    uint8_t wager_enabled;     // 0: off, 1: on
} CreateRoomRequest;

typedef struct PACKED {
    uint32_t room_id;
    char room_code[11];
    uint32_t host_id;
    uint64_t created_at;
} CreateRoomResponse;

// ===== SET RULES =====
typedef struct PACKED {
    uint32_t room_id;
    uint8_t mode;
    uint8_t max_players;
    uint16_t round_time;
    uint8_t wager_enabled;
} SetRuleRequest;

typedef struct PACKED {
    uint8_t mode;
    uint8_t max_players;
    uint16_t round_time;
    uint8_t wager_enabled;
    uint64_t updated_at;
} RulesUpdatedNotification;

// ===== KICK MEMBER =====
typedef struct PACKED {
    uint32_t room_id;
    uint32_t target_user_id;
} KickRequest;

typedef struct PACKED {
    uint32_t room_id;
    char reason[128];
    uint64_t kicked_at;
} KickedNotification;

typedef struct PACKED {
    uint32_t user_id;
    char username[32];
    uint8_t new_player_count;
} PlayerLeftNotification;

// ===== DELETE ROOM =====
typedef struct PACKED {
    uint32_t room_id;
} DeleteRoomRequest;

typedef struct PACKED {
    uint32_t room_id;
    char reason[128];
    uint64_t closed_at;
} RoomClosedNotification;

// ===== START GAME =====
typedef struct PACKED {
    uint32_t room_id;
} StartGameRequest;

typedef struct PACKED {
    uint32_t match_id;
    uint8_t countdown;
    uint64_t server_timestamp;
} GameStartingNotification;

typedef struct PACKED {
    uint8_t round_id;
    uint16_t time_limit;
    uint64_t round_start_time;
    uint16_t data_len;
    char data[];  // Variable length
} RoundStartNotification;

// ===== ERROR RESPONSE =====
typedef struct PACKED {
    char message[256];
} ErrorResponse;

#endif // PAYLOADS_H
