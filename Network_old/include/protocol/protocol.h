#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

//==============================================================================
// PROTOCOL CONSTANTS
//==============================================================================
#define MAGIC_NUMBER 0x4347
#define PROTOCOL_VERSION 0x01
#define HEADER_SIZE 16
#define BUFF_SIZE 4096

//==============================================================================
// COMMAND CODES
//==============================================================================

// Authentication & Account
#define CMD_LOGIN_REQ       0x0100
#define CMD_REGISTER_REQ    0x0101
#define CMD_LOGOUT_REQ      0x0102
#define CMD_RECONNECT       0x0103

// Lobby (Room Management)
#define CMD_CREATE_ROOM     0x0200
#define CMD_JOIN_ROOM       0x0201
#define CMD_LEAVE_ROOM      0x0202
#define CMD_READY           0x0203
#define CMD_KICK            0x0204
#define CMD_INVITE_FRIEND   0x0205
#define CMD_SET_RULE        0x0206

// Gameplay
#define CMD_START_GAME      0x0300
#define CMD_ANSWER_QUIZ     0x0301
#define CMD_BID             0x0302
#define CMD_SPIN            0x0303
#define CMD_FORFEIT         0x0304
#define CMD_BONUS           0x0305

// Social & History
#define CMD_CHAT            0x0500
#define CMD_FRIEND_ADD      0x0501
#define CMD_HIST            0x0502
#define CMD_REPLAY          0x0503
#define CMD_LEAD            0x0504


// System
#define CMD_HEARTBEAT       0x0001

//==============================================================================
// RESPONSE CODES
//==============================================================================

// Success
#define RES_SUCCESS         200
#define RES_LOGIN_OK        201
#define RES_HEARTBEAT_OK    210
#define RES_ROOM_JOINED     301

// Error
#define ERR_BAD_REQUEST     400
#define ERR_NOT_LOGGED_IN   401
#define ERR_INVALID_USERNAME 402
#define ERR_ROOM_FULL       403
#define ERR_GAME_STARTED    404
#define ERR_PAYLOAD_LARGE   405
#define ERR_NOT_HOST        406
#define ERR_TIMEOUT         408
#define ERR_SERVER_ERROR    500
#define ERR_SERVICE_UNAVAILABLE 501

//==============================================================================
// NOTIFICATION CODES
//==============================================================================

#define NTF_PLAYER_JOINED   700
#define NTF_PLAYER_LEFT     701
#define NTF_PLAYER_LIST     702
#define NTF_ROUND_START     703
#define NTF_ROUND_END       704
#define NTF_SCOREBOARD      705
#define NTF_ELIMINATION     706
#define NTF_GAME_END        707
#define NTF_CHAT_MSG        710
#define NTF_INVITATION      711

//==============================================================================
// PROTOCOL STRUCTURES
//==============================================================================

#if defined(__GNUC__) || defined(__clang__)
#define PACKED __attribute__((packed))
#else
#define PACKED
#pragma pack(push, 1)
#endif

typedef struct PACKED {
    uint16_t magic;
    uint8_t version;
    uint8_t flags;
    uint16_t command;
    uint16_t reserved;
    uint32_t seq_num;
    uint32_t length;
} MessageHeader;

// Payload structures
typedef struct PACKED {
    char username[32];
    char password[64];
} AuthRequestPayload;

typedef struct PACKED {
    uint32_t user_id;
    uint32_t session_id;
    uint32_t balance;
    char display_name[32];
} AuthSuccessPayload;

typedef struct PACKED {
    uint32_t room_id;
} JoinRoomPayload;

typedef struct PACKED {
    uint8_t max_players;
    uint8_t game_mode;
    uint16_t reserved;
} CreateRoomPayload;

typedef struct PACKED {
    uint32_t bid_price;
} BidPayload;

typedef struct PACKED {
    uint8_t answer_index;
} QuizAnswerPayload;

typedef struct PACKED {
    uint32_t room_id;
    uint8_t player_count;
    uint8_t max_players;
    uint8_t game_mode;
    uint8_t is_playing;
    char host_name[32];
} RoomInfoPayload;

typedef struct PACKED {
    uint8_t round_id;
    uint16_t time_limit;
    uint16_t data_len;
    char data[];
} RoundStartPayload;




#if !defined(__GNUC__) && !defined(__clang__)
#pragma pack(pop)
#endif

//==============================================================================
// ENUMERATIONS
//==============================================================================

typedef enum {
    STATE_UNAUTHENTICATED,
    STATE_AUTHENTICATED,
    STATE_IN_ROOM,
    STATE_PLAYING
} ConnectionState;

typedef enum {
    GAME_MODE_SCORING,
    GAME_MODE_ELIMINATION
} GameMode;

typedef enum {
    ROUND_QUIZ = 1,
    ROUND_BID = 2,
    ROUND_WHEEL = 3
} RoundType;


//==============================================================================
// FUNCTION DECLARATIONS
//==============================================================================

int send_response(int sockfd, uint16_t command, const char *payload, uint32_t length);
int receive_header(int client_idx, MessageHeader *header);
int receive_payload(int client_idx, char *payload, uint32_t length);
int validate_header(MessageHeader *header);
int validate_command_for_state(uint16_t command, ConnectionState state);

#endif // PROTOCOL_H