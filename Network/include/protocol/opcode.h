//==============================================================================
// COMMAND CODES
//==============================================================================

// Authentication & Account
#define CMD_LOGIN_REQ       0x0100
#define CMD_REGISTER_REQ    0x0101
#define CMD_LOGOUT_REQ      0x0102
#define CMD_RECONNECT       0x0103
// Profile / Player Settings
#define CMD_UPDATE_PROFILE   0x0104
#define CMD_GET_PROFILE      0x0105

// Lobby (Room Management)
#define CMD_CREATE_ROOM     0x0200
#define CMD_JOIN_ROOM       0x0201
#define CMD_LEAVE_ROOM      0x0202
#define CMD_READY           0x0203
#define CMD_KICK            0x0204
#define CMD_INVITE_FRIEND   0x0205
#define CMD_SET_RULE        0x0206
#define CMD_CLOSE_ROOM      0x0207  // Host closes room

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
#define RES_ROOM_CREATED    220
#define RES_ROOM_JOINED     221
#define RES_ROOM_LEFT       222
#define RES_ROOM_CLOSED     223
#define RES_RULES_UPDATED   224
#define RES_MEMBER_KICKED   225
#define RES_GAME_STARTED    301
// Profile
#define RES_PROFILE_UPDATED  226
#define RES_PROFILE_FOUND    227

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
#define NTF_GAME_START      708
#define NTF_CHAT_MSG        710
#define NTF_INVITATION      711
#define NTF_PLAYER_READY    712
#define NTF_RULES_CHANGED   713
#define NTF_MEMBER_KICKED   714
#define NTF_ROOM_CLOSED     715

// Round 1 - Quiz (0x0601 - 0x061F)
#define OP_C2S_ROUND1_READY       0x0601  // Client ready to start
#define OP_S2C_ROUND1_START       0x0611  // Server: round started
#define OP_C2S_ROUND1_GET_QUESTION 0x0602 // Request next question
#define OP_S2C_ROUND1_QUESTION    0x0612  // Server sends question
#define OP_C2S_ROUND1_ANSWER      0x0604  // Submit answer
#define OP_S2C_ROUND1_RESULT      0x0614  // Answer result
#define OP_C2S_ROUND1_END         0x0605  // End round (player finished)
#define OP_S2C_ROUND1_END         0x0615  // Round ended (all players done)

// Round 1 - Sync opcodes
#define OP_C2S_ROUND1_PLAYER_READY  0x0606  // Player clicks ready
#define OP_S2C_ROUND1_READY_STATUS  0x0616  // Broadcast ready status
#define OP_S2C_ROUND1_ALL_READY     0x0617  // All players ready, game starts
#define OP_C2S_ROUND1_FINISHED      0x0607  // Player finished all questions
#define OP_S2C_ROUND1_WAITING       0x0618  // Waiting for other players
#define OP_S2C_ROUND1_ALL_FINISHED  0x0619  // All players finished, show results