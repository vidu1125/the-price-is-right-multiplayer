#ifndef COMMANDS_H
#define COMMANDS_H

// ===== CLIENT COMMANDS =====
typedef enum {
    // Authentication (0x01xx)
    CMD_LOGIN_REQ       = 0x0100,
    CMD_REGISTER_REQ    = 0x0101,
    CMD_LOGOUT_REQ      = 0x0102,
    
    // Lobby & Room (0x02xx)
    CMD_CREATE_ROOM     = 0x0200,
    CMD_JOIN_ROOM       = 0x0201,
    CMD_LEAVE_ROOM      = 0x0202,
    CMD_READY           = 0x0203,
    CMD_KICK            = 0x0204,
    CMD_SET_RULE        = 0x0206,
    CMD_DELETE_ROOM     = 0x0207,
    
    // Gameplay (0x03xx)
    CMD_START_GAME      = 0x0300,
    CMD_ANSWER_QUIZ     = 0x0301,
    CMD_BID             = 0x0302,
    CMD_SPIN            = 0x0303,
    CMD_FORFEIT         = 0x0304,
    
    // System (0x00xx)
    CMD_HEARTBEAT       = 0x0001,
    CMD_TIME_SYNC       = 0x0002
} ClientCommand;

// ===== SERVER RESPONSES =====
typedef enum {
    // Success (2xx)
    RES_SUCCESS         = 200,
    RES_LOGIN_OK        = 201,
    RES_ROOM_CREATED    = 202,
    
    // Client Errors (4xx)
    ERR_BAD_REQUEST     = 400,
    ERR_NOT_LOGGED_IN   = 401,
    ERR_FORBIDDEN       = 403,
    ERR_ROOM_FULL       = 403,
    ERR_GAME_STARTED    = 404,
    ERR_NOT_HOST        = 406,
    ERR_TIMEOUT         = 408,
    
    // Server Errors (5xx)
    ERR_SERVER_ERROR    = 500
} ServerResponse;

// ===== SERVER NOTIFICATIONS =====
typedef enum {
    // Room events (7xx)
    NTF_PLAYER_JOINED   = 700,
    NTF_PLAYER_LEFT     = 701,
    NTF_PLAYER_LIST     = 702,
    NTF_RULES_UPDATED   = 703,
    NTF_KICKED          = 704,
    NTF_ROOM_CLOSED     = 705,
    
    // Game flow (71x)
    NTF_GAME_STARTING   = 710,
    NTF_ROUND_START     = 711,
    NTF_ROUND_END       = 712,
    NTF_SCOREBOARD      = 713,
    NTF_GAME_END        = 714
} ServerNotification;

#endif // COMMANDS_H
