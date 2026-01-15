import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import '../network/tcp_client.dart';
import '../network/dispatcher.dart';
import '../core/command.dart';
import '../core/protocol.dart';
import 'service_locator.dart';

/// Game state event types
enum GameEventType {
  // General game events
  roundStart,          // NTF_ROUND_START (703)
  roundEnd,            // NTF_ROUND_END (704)
  scoreboard,          // NTF_SCOREBOARD (705)
  elimination,         // NTF_ELIMINATION (706)
  gameEnd,             // NTF_GAME_END (707)
  
  // Round 1 MCQ events
  round1Question,      // OP_S2C_ROUND1_QUESTION (0x0612)
  round1Result,        // OP_S2C_ROUND1_RESULT (0x0614)
  round1AllReady,      // OP_S2C_ROUND1_ALL_READY (0x0617)
  round1AllFinished,   // OP_S2C_ROUND1_ALL_FINISHED (0x0619)
  round1ReadyStatus,   // OP_S2C_ROUND1_READY_STATUS (0x0616)
  round1Waiting,       // OP_S2C_ROUND1_WAITING (0x0618)
  
  // Round 2 Bidding events
  round2Product,       // OP_S2C_ROUND2_PRODUCT (0x0632)
  round2BidAck,        // OP_S2C_ROUND2_BID_ACK (0x0633)
  round2TurnResult,    // OP_S2C_ROUND2_TURN_RESULT (0x0634)
  round2AllReady,      // OP_S2C_ROUND2_ALL_READY (0x0631)
  round2AllFinished,   // OP_S2C_ROUND2_ALL_FINISHED (0x0635)
  round2ReadyStatus,   // OP_S2C_ROUND2_READY_STATUS (0x0630)
  
  // Round 3 Wheel events
  round3SpinResult,    // OP_S2C_ROUND3_SPIN_RESULT (0x0652)
  round3DecisionPrompt,// OP_S2C_ROUND3_DECISION_ACK (0x0653)
  round3FinalResult,   // OP_S2C_ROUND3_FINAL_RESULT (0x0654)
  round3AllReady,      // OP_S2C_ROUND3_ALL_READY (0x0651)
  round3AllFinished,   // OP_S2C_ROUND3_ALL_FINISHED (0x0655)
  round3ReadyStatus,   // OP_S2C_ROUND3_READY_STATUS (0x0650)
  
  // Player connection events
  playerDisconnected,  // NTF_PLAYER_LEFT (701)
  playerReconnected,   // NTF_PLAYER_LIST (702) - used for reconnect
}

class GameEvent {
  final GameEventType type;
  final dynamic data;
  GameEvent(this.type, [this.data]);
}

/// Service for managing in-game state and communication
class GameStateService {
  final TcpClient client;
  final Dispatcher dispatcher;
  final _eventController = StreamController<GameEvent>.broadcast();

  Stream<GameEvent> get events => _eventController.stream;

  // Round 1 opcodes (MCQ)
  static const int opC2SRound1Ready = 0x0601;
  static const int opS2CRound1Start = 0x0611;
  static const int opC2SRound1GetQuestion = 0x0602;
  static const int opS2CRound1Question = 0x0612;
  static const int opC2SRound1Answer = 0x0604;
  static const int opS2CRound1Result = 0x0614;
  static const int opC2SRound1PlayerReady = 0x0606;
  static const int opS2CRound1ReadyStatus = 0x0616;
  static const int opS2CRound1AllReady = 0x0617;
  static const int opC2SRound1Finished = 0x0607;
  static const int opS2CRound1Waiting = 0x0618;
  static const int opS2CRound1AllFinished = 0x0619;

  // Round 2 opcodes (Bidding)
  static const int opC2SRound2Ready = 0x0620;
  static const int opC2SRound2PlayerReady = 0x0621;
  static const int opS2CRound2ReadyStatus = 0x0630;
  static const int opS2CRound2AllReady = 0x0631;
  static const int opC2SRound2GetProduct = 0x0622;
  static const int opS2CRound2Product = 0x0632;
  static const int opC2SRound2Bid = 0x0623;
  static const int opS2CRound2BidAck = 0x0633;
  static const int opS2CRound2TurnResult = 0x0634;
  static const int opS2CRound2AllFinished = 0x0635;

  // Round 3 opcodes (Wheel)
  static const int opC2SRound3Ready = 0x0640;
  static const int opC2SRound3PlayerReady = 0x0641;
  static const int opS2CRound3ReadyStatus = 0x0650;
  static const int opS2CRound3AllReady = 0x0651;
  static const int opC2SRound3Spin = 0x0642;
  static const int opS2CRound3SpinResult = 0x0652;
  static const int opC2SRound3Decision = 0x0643;
  static const int opS2CRound3DecisionAck = 0x0653;
  static const int opS2CRound3FinalResult = 0x0654;
  static const int opS2CRound3AllFinished = 0x0655;

  GameStateService(this.client, this.dispatcher) {
    _registerHandlers();
  }

  void _registerHandlers() {
    // General game notification handlers
    dispatcher.register(Command.ntfRoundStart, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.roundStart, json));
    });

    dispatcher.register(Command.ntfRoundEnd, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.roundEnd, json));
    });

    dispatcher.register(Command.ntfScoreboard, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.scoreboard, json));
    });

    dispatcher.register(Command.ntfElimination, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.elimination, json));
    });

    dispatcher.register(Command.ntfGameEnd, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.gameEnd, json));
    });

    // Player connection handlers
    dispatcher.register(Command.ntfPlayerLeft, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.playerDisconnected, json));
    });

    dispatcher.register(Command.ntfPlayerList, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.playerReconnected, json));
    });

    // ========== ROUND 1 HANDLERS ==========
    dispatcher.register(opS2CRound1Question, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      print("[GameStateService] Round1 Question received: $json");
      _eventController.add(GameEvent(GameEventType.round1Question, json));
    });

    dispatcher.register(opS2CRound1Result, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round1Result, json));
    });

    dispatcher.register(opS2CRound1AllReady, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round1AllReady, json));
    });

    dispatcher.register(opS2CRound1AllFinished, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round1AllFinished, json));
    });

    dispatcher.register(opS2CRound1ReadyStatus, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round1ReadyStatus, json));
    });

    dispatcher.register(opS2CRound1Waiting, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round1Waiting, json));
    });

    // ========== ROUND 2 HANDLERS ==========
    dispatcher.register(opS2CRound2Product, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      print("[GameStateService] Round2 Product received: $json");
      _eventController.add(GameEvent(GameEventType.round2Product, json));
    });

    dispatcher.register(opS2CRound2BidAck, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round2BidAck, json));
    });

    dispatcher.register(opS2CRound2TurnResult, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round2TurnResult, json));
    });

    dispatcher.register(opS2CRound2AllReady, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round2AllReady, json));
    });

    dispatcher.register(opS2CRound2AllFinished, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round2AllFinished, json));
    });

    dispatcher.register(opS2CRound2ReadyStatus, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round2ReadyStatus, json));
    });

    // ========== ROUND 3 HANDLERS ==========
    dispatcher.register(opS2CRound3SpinResult, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      print("[GameStateService] Round3 Spin result: $json");
      _eventController.add(GameEvent(GameEventType.round3SpinResult, json));
    });

    dispatcher.register(opS2CRound3DecisionAck, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round3DecisionPrompt, json));
    });

    dispatcher.register(opS2CRound3FinalResult, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round3FinalResult, json));
    });

    dispatcher.register(opS2CRound3AllReady, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round3AllReady, json));
    });

    dispatcher.register(opS2CRound3AllFinished, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round3AllFinished, json));
    });

    dispatcher.register(opS2CRound3ReadyStatus, (msg) {
      var json = Protocol.decodeJson(msg.payload);
      _eventController.add(GameEvent(GameEventType.round3ReadyStatus, json));
    });
  }

  // ===========================
  // ROUND 1: MCQ Methods
  // ===========================

  /// Send ready signal for Round 1
  void sendRound1Ready(int matchId) async {
    // Get player ID from auth service
    final authService = ServiceLocator.authService;
    final authState = await authService.getAuthState();
    final playerId = int.tryParse(authState['accountId'] ?? '0') ?? 0;
    
    final buffer = ByteData(8);
    buffer.setUint32(0, matchId, Endian.big);
    buffer.setUint32(4, playerId, Endian.big);
    
    print("[GameStateService] Sending ROUND1_PLAYER_READY for match $matchId, player $playerId");
    
    final packet = Protocol.buildPacket(
      command: opC2SRound1PlayerReady,  // Changed from opC2SRound1Ready
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  /// Request next question in Round 1
  void requestRound1Question(int matchId, {int questionIndex = 0}) {
    final buffer = ByteData(8);
    buffer.setUint32(0, matchId, Endian.big);
    buffer.setUint32(4, questionIndex, Endian.big);
    
    print("[GameStateService] Requesting question $questionIndex for match $matchId");
    
    final packet = Protocol.buildPacket(
      command: opC2SRound1GetQuestion,
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  /// Submit answer for Round 1 MCQ
  void submitRound1Answer(int matchId, int questionIndex, int choiceIndex, {int timeTakenMs = 0}) {
    final buffer = ByteData(13);
    buffer.setUint32(0, matchId, Endian.big);
    buffer.setUint32(4, questionIndex, Endian.big);
    buffer.setUint8(8, choiceIndex);
    buffer.setUint32(9, timeTakenMs, Endian.big);
    
    print("[GameStateService] Submitting answer: match=$matchId, question=$questionIndex, choice=$choiceIndex, time=${timeTakenMs}ms");
    
    final packet = Protocol.buildPacket(
      command: opC2SRound1Answer,
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  /// Signal player ready for Round 1
  void sendRound1PlayerReady(int matchId, int playerId) {
    final buffer = ByteData(8);
    buffer.setUint32(0, matchId, Endian.big);
    buffer.setUint32(4, playerId, Endian.big);
    
    final packet = Protocol.buildPacket(
      command: opC2SRound1PlayerReady,
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  /// Signal player finished Round 1
  void sendRound1Finished(int matchId, int playerId) {
    final buffer = ByteData(8);
    buffer.setUint32(0, matchId, Endian.big);
    buffer.setUint32(4, playerId, Endian.big);
    
    final packet = Protocol.buildPacket(
      command: opC2SRound1Finished,
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  // ===========================
  // ROUND 2: Bidding Methods
  // ===========================

  /// Signal player ready for Round 2
  void sendRound2PlayerReady(int matchId, int playerId) {
    final buffer = ByteData(8);
    buffer.setUint32(0, matchId, Endian.big);
    buffer.setUint32(4, playerId, Endian.big);
    
    print("[GameStateService] Sending ROUND2_PLAYER_READY for match $matchId");
    
    final packet = Protocol.buildPacket(
      command: opC2SRound2PlayerReady,
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  /// Request product information in Round 2
  void requestRound2Product(int matchId, int productIndex) {
    final buffer = ByteData(8);
    buffer.setUint32(0, matchId, Endian.big);
    buffer.setUint32(4, productIndex, Endian.big);
    
    print("[GameStateService] Requesting product $productIndex for match $matchId");
    
    final packet = Protocol.buildPacket(
      command: opC2SRound2GetProduct,
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  /// Submit bid for Round 2
  void submitRound2Bid(int matchId, int productIndex, int bidValue) {
    final buffer = ByteData(16);
    buffer.setUint32(0, matchId, Endian.big);
    buffer.setUint32(4, productIndex, Endian.big);
    buffer.setInt64(8, bidValue, Endian.big); // int64 for bid value
    
    print("[GameStateService] Submitting bid: match=$matchId, product=$productIndex, bid=$bidValue");
    
    final packet = Protocol.buildPacket(
      command: opC2SRound2Bid,
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  // ===========================
  // ROUND 3: Wheel Methods
  // ===========================

  /// Signal player ready for Round 3
  void sendRound3PlayerReady(int matchId, int playerId) {
    final buffer = ByteData(8);
    buffer.setUint32(0, matchId, Endian.big);
    buffer.setUint32(4, playerId, Endian.big);
    
    print("[GameStateService] Sending ROUND3_PLAYER_READY for match $matchId");
    
    final packet = Protocol.buildPacket(
      command: opC2SRound3PlayerReady,
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  /// Request spin in Round 3
  void requestRound3Spin(int matchId) {
    final buffer = ByteData(4);
    buffer.setUint32(0, matchId, Endian.big);
    
    print("[GameStateService] Requesting spin for match $matchId");
    
    final packet = Protocol.buildPacket(
      command: opC2SRound3Spin,
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  /// Submit decision in Round 3 (continue or stop)
  void submitRound3Decision(int matchId, bool continueSpinning) {
    final buffer = ByteData(5);
    buffer.setUint32(0, matchId, Endian.big);
    buffer.setUint8(4, continueSpinning ? 1 : 0);
    
    print("[GameStateService] Submitting decision: match=$matchId, continue=$continueSpinning");
    
    final packet = Protocol.buildPacket(
      command: opC2SRound3Decision,
      seqNum: 0,
      payload: buffer.buffer.asUint8List(),
    );
    client.send(packet);
  }

  void dispose() {
    _eventController.close();
  }
}
