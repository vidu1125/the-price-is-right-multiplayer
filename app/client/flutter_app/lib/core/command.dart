// lib/core/command.dart

class Command {
  // === COMMAND CODES ===

  // System
  static const int heartbeat = 0x0001;

  // Authentication & Account
  static const int loginReq = 0x0100;
  static const int registerReq = 0x0101;
  static const int logoutReq = 0x0102;
  static const int reconnect = 0x0103;
  static const int cmdUpdateProfile = 0x0104; 
  static const int cmdGetProfile = 0x0105; 

  // Lobby (Room Management)
  static const int createRoom = 0x0200;
  static const int joinRoom = 0x0201;
  static const int leaveRoom = 0x0202;
  static const int ready = 0x0203;
  static const int kick = 0x0204;
  static const int inviteFriend = 0x0205;
  static const int setRule = 0x0206;
  static const int closeRoom = 0x0207;

  // Gameplay
  static const int startGame = 0x0300;
  static const int answerQuiz = 0x0301;
  static const int bid = 0x0302;
  static const int spin = 0x0303;
  static const int forfeit = 0x0304;
  static const int bonus = 0x0305;

  // Social & History
  static const int chat = 0x0500;
  static const int friendAdd = 0x0501;
  static const int history = 0x0502;    // CMD_HIST
  static const int replay = 0x0503;     // CMD_REPLAY
  static const int leaderboard = 0x0504; // CMD_LEAD

  // === RESPONSE CODES ===

  // Success
  static const int resSuccess = 200;
  static const int resLoginOk = 201;
  static const int resHeartbeatOk = 210;
  static const int resRoomCreated = 220;
  static const int resRoomJoined = 221;
  static const int resRoomLeft = 222;
  static const int resRoomClosed = 223;
  static const int resRulesUpdated = 224;
  static const int resMemberKicked = 225;
  static const int resProfileUpdated = 226;
  static const int resProfileFound = 227;
  static const int resGameStarted = 301;

  // Error
  static const int errBadRequest = 400;
  static const int errNotLoggedIn = 401;
  static const int errInvalidUsername = 402;
  static const int errRoomFull = 403;
  static const int errGameStarted = 404;
  static const int errPayloadLarge = 405;
  static const int errNotHost = 406;
  static const int errTimeout = 408;
  static const int errServerError = 500;
  static const int errServiceUnavailable = 501;

  // === NOTIFICATION CODES ===

  static const int ntfPlayerJoined = 700;
  static const int ntfPlayerLeft = 701;
  static const int ntfPlayerList = 702;
  static const int ntfRoundStart = 703;
  static const int ntfRoundEnd = 704;
  static const int ntfScoreboard = 705;
  static const int ntfElimination = 706;
  static const int ntfGameEnd = 707;
  static const int ntfGameStart = 708;
  static const int ntfChatMsg = 710;
  static const int ntfInvitation = 711;
  static const int ntfPlayerReady = 712;
  static const int ntfRulesChanged = 713;
  static const int ntfMemberKicked = 714;
  static const int ntfRoomClosed = 715;
}
