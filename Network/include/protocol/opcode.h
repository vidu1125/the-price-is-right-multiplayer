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
#define CMD_CHANGE_PASSWORD  0x0106

// Lobby (Room Management)
#define CMD_CREATE_ROOM     0x0200
#define CMD_JOIN_ROOM       0x0201
#define CMD_LEAVE_ROOM      0x0202
#define CMD_READY           0x0203
#define CMD_KICK            0x0204
#define CMD_INVITE_FRIEND   0x0205
#define CMD_SET_RULE        0x0206
#define CMD_CLOSE_ROOM      0x0207  // Host closes room
#define CMD_GET_ROOM_LIST   0x0208  // Get list of waiting rooms

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
// Social Extended (0x0505 - 0x050F)
#define CMD_FRIEND_ACCEPT   0x0505  // Accept friend request
#define CMD_FRIEND_REJECT   0x0506  // Reject friend request
#define CMD_FRIEND_REMOVE   0x0507  // Remove friend
#define CMD_FRIEND_LIST     0x0508  // Get list of friends with status
#define CMD_FRIEND_REQUESTS 0x0509  // Get pending friend requests
#define CMD_STATUS_UPDATE   0x050A  // Update presence status (ONLINE_IDLE, PLAYING, OFFLINE)
#define CMD_GET_FRIEND_STATUS 0x050B // Get specific friend's status
#define CMD_SEARCH_USER     0x050C  // Search users by name/email


// System
#define CMD_HEARTBEAT       0x0001

//==============================================================================
// RESPONSE CODES
//==============================================================================

// Success
#define RES_SUCCESS         0x00C8  // 200
#define RES_LOGIN_OK        0x00C9  // 201
#define RES_HEARTBEAT_OK    0x00D2  // 210
#define RES_ROOM_CREATED    0x00DC  // 220
#define RES_ROOM_JOINED     0x00DD  // 221
#define RES_ROOM_LEFT       0x00DE  // 222
#define RES_ROOM_CLOSED     0x00DF  // 223
#define RES_RULES_UPDATED   0x00E0  // 224
#define RES_MEMBER_KICKED   0x00E1  // 225
#define RES_ROOM_LIST       0x00E4  // 228
#define RES_GAME_STARTED    0x012D  // 301
#define RES_READY_OK        0x00EC  // 236
// Profile
#define RES_PROFILE_UPDATED 0x00E2  // 226
#define RES_PROFILE_FOUND   0x00E3  // 227
#define RES_PASSWORD_CHANGED 0x00E4 // 228
// Social
#define RES_FRIEND_ADDED    0x00E5  // 229
#define RES_FRIEND_REQUEST_ACCEPTED 0x00E6  // 230
#define RES_FRIEND_REQUEST_REJECTED 0x00E7  // 231
#define RES_FRIEND_REMOVED  0x00E8  // 232
#define RES_FRIEND_LIST     0x00E9  // 233
#define RES_FRIEND_REQUESTS 0x00EA  // 234
#define RES_STATUS_UPDATED  0x00EB  // 235
// Error
#define ERR_BAD_REQUEST     0x0190  // 400
#define ERR_NOT_LOGGED_IN   0x0191  // 401
#define ERR_INVALID_USERNAME 0x0192  // 402
#define ERR_ROOM_FULL       0x0193  // 403
#define ERR_GAME_STARTED    0x0194  // 404
#define ERR_PAYLOAD_LARGE   0x0195  // 405
#define ERR_NOT_HOST        0x0196  // 406
#define ERR_TIMEOUT         0x0198  // 408

// Social Errors (40x - 41x)
#define ERR_FORBIDDEN       0x0193  // 403
#define ERR_NOT_FOUND       0x0194  // 404
#define ERR_CONFLICT        0x0199  // 409
#define ERR_FRIEND_NOT_FOUND 0x019A // 410
#define ERR_ALREADY_FRIENDS 0x019B // 411
#define ERR_FRIEND_REQ_DUPLICATE 0x019C // 412
#define ERR_SELF_ADD        0x019D // 413
#define ERR_FRIEND_REQ_NOT_FOUND 0x019E // 414

#define ERR_SERVER_ERROR    0x01F4  // 500
#define ERR_SERVICE_UNAVAILABLE 0x01F5  // 501

//==============================================================================
// NOTIFICATION CODES
//==============================================================================

#define NTF_PLAYER_JOINED   0x02BC  //700
#define NTF_PLAYER_LEFT     0x02BD  //701
#define NTF_PLAYER_LIST     0x02BE  //702
#define NTF_ROUND_START     0x02BF  //703
#define NTF_ROUND_END       0x02C0  //704
#define NTF_SCOREBOARD      0x02C1  //705
#define NTF_ELIMINATION     0x02C2  //706
#define NTF_GAME_END        0x02C3  //707
#define NTF_GAME_START      0x02C4  //708
#define NTF_CHAT_MSG        0x02C6  //710
#define NTF_INVITATION      0x02C7  //711
#define NTF_PLAYER_READY    0x02C8  //712
#define NTF_RULES_CHANGED   0x02C9  //713
#define NTF_MEMBER_KICKED   0x02CA  //714
#define NTF_ROOM_CLOSED     0x02CB  //715
#define NTF_HOST_CHANGED    0x02D1  //721
// Social Notifications (71x)
#define NTF_FRIEND_REQUEST  0x02CC  // 716 Friend request received
#define NTF_FRIEND_ACCEPTED 0x02CD  // 717 Friend request accepted
#define NTF_FRIEND_STATUS   0x02CE  // 718 Friend's presence status changed
#define NTF_FRIEND_ADDED    0x02CF  // 719 Friend added (mutual)
#define NTF_FRIEND_REMOVED  0x02D0  // 720 Friend removed

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