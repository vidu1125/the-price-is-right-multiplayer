import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import '../network/tcp_client.dart';
import '../network/dispatcher.dart';
import '../core/command.dart';
import '../models/room.dart';
import '../core/protocol.dart';
import '../core/config.dart';

enum RoomEventType {
  playerJoined,
  playerLeft,
  gameStarted,
  roomClosed,
  memberKicked,
  rulesChanged,
  playerReady,
  invitationReceived,
  hostChanged
}

class RoomEvent {
  final RoomEventType type;
  final dynamic data;
  RoomEvent(this.type, [this.data]);
}

class RoomService {
  final TcpClient client;
  final Dispatcher dispatcher;
  final _eventController = StreamController<RoomEvent>.broadcast();

  Stream<RoomEvent> get events => _eventController.stream;

  String get _apiBaseUrl {
      return "http://${AppConfig.serverHost}:${AppConfig.serverPort}/api";
  }

  RoomService(this.client, this.dispatcher) {
    _registerHandlers();
  }

  void _registerHandlers() {
    dispatcher.register(Command.ntfPlayerJoined, (msg) {
        var json = Protocol.decodeJson(msg.payload);
        _eventController.add(RoomEvent(RoomEventType.playerJoined, json));
    });

    dispatcher.register(Command.ntfPlayerLeft, (msg) {
        var json = Protocol.decodeJson(msg.payload);
        _eventController.add(RoomEvent(RoomEventType.playerLeft, json));
    });

    dispatcher.register(Command.ntfPlayerList, (msg) {
        // Server sends full player list (usually after join)
        // We can ignore this since we get the full list in RES_ROOM_JOINED
        print("[RoomService] Received player list notification");
    });
    
    dispatcher.register(Command.ntfGameStart, (msg) {
        print("[RoomService] 🎮 NTF_GAME_START received!");
        var json = Protocol.decodeJson(msg.payload);
        print("[RoomService] Game start payload: $json");
        _eventController.add(RoomEvent(RoomEventType.gameStarted, json));
        print("[RoomService] gameStarted event dispatched!");
    });

    dispatcher.register(Command.ntfRoomClosed, (_) {
        _eventController.add(RoomEvent(RoomEventType.roomClosed));
    });

    dispatcher.register(Command.ntfMemberKicked, (_) {
        _eventController.add(RoomEvent(RoomEventType.memberKicked));
    });
    
    dispatcher.register(Command.ntfRulesChanged, (msg) {
        var json = Protocol.decodeJson(msg.payload);
        _eventController.add(RoomEvent(RoomEventType.rulesChanged, json));
    });

    dispatcher.register(Command.ntfPlayerReady, (msg) {
        var json = Protocol.decodeJson(msg.payload);
        _eventController.add(RoomEvent(RoomEventType.playerReady, json));
    });

    dispatcher.register(Command.ntfInvitation, (msg) {
        var json = Protocol.decodeJson(msg.payload);
        _eventController.add(RoomEvent(RoomEventType.invitationReceived, json));
    });

    dispatcher.register(Command.ntfHostChanged, (msg) {
        var json = Protocol.decodeJson(msg.payload);
        _eventController.add(RoomEvent(RoomEventType.hostChanged, json));
    });
  }

  Future<bool> inviteFriend(int targetId, int roomId) async {
      try {
          final buffer = ByteData(8);
          buffer.setInt32(0, targetId, Endian.big);
          buffer.setUint32(4, roomId, Endian.big);
          
          final response = await client.request(Command.inviteFriend, payload: buffer.buffer.asUint8List());
          return response.command == Command.resSuccess;
      } catch (e) {
          print("Error inviting friend: $e");
          return false;
      }
  }

  Future<Map<String, dynamic>> createRoom({
    required String name,
    required String visibility,
    required String mode,
    required int maxPlayers,
    required bool wager
  }) async {
    final buffer = ByteData(36);
    
    // name [32 bytes] - match JS logic of using 32 bytes max and null terminating
    // We create a temp list of 32 zeros
    final nameBuffer = Uint8List(32);
    final encodedName = utf8.encode(name);
    // Copy bytes, leaving at least 1 null byte if length >= 32 (JS logic: truncated = str.substring(0, 31))
    // However, JS uses TextEncoder which might output fewer bytes for multi-byte chars.
    // Darts utf8.encode returns bytes. 
    // We should copy up to 31 bytes to guarantee null termination if we want to mimic JS exactly?
    // JS: const truncated = str.substring(0, maxLength - 1); -> This truncates CHARACTERS safely.
    // Then const bytes = encoder.encode(truncated);
    
    // Let's emulate that:
    String safeName = name;
    if (safeName.length > 31) {
        safeName = safeName.substring(0, 31);
    }
    
    final nameBytes = utf8.encode(safeName);
    // Copy into nameBuffer (which is already zeroed by default in Dart Uint8List?)
    // Yes, Uint8List defaults to 0.
    for (var i = 0; i < nameBytes.length && i < 32; i++) {
        nameBuffer[i] = nameBytes[i];
    }
    
    // Set buffer with nameBuffer
    for (int i = 0; i < 32; i++) {
        buffer.setUint8(i, nameBuffer[i]);
    }
    
    // mode (offset 32)
    int modeVal = 1; // SCORING
    if (mode.toLowerCase() == "eliminate" || mode.toLowerCase() == "elimination") modeVal = 0;
    buffer.setUint8(32, modeVal);
    
    // max_players (offset 33)
    buffer.setUint8(33, maxPlayers);
    
    // visibility (offset 34)
    int visibilityVal = (visibility == "private") ? 1 : 0;
    buffer.setUint8(34, visibilityVal);
    
    // wager_mode (offset 35)
    int wagerVal = wager ? 1 : 0;
    buffer.setUint8(35, wagerVal);

    // Debug Log
    String hexPayload = buffer.buffer.asUint8List().map((b) => "0x${b.toRadixString(16).padLeft(2, '0')}").join(" ");
    print("[CLIENT] [CREATE_ROOM] Sending 36 bytes: $hexPayload");

    try {
        final response = await client.request(Command.createRoom, payload: buffer.buffer.asUint8List());
        if (response.command == Command.resRoomCreated) {
             final respData = ByteData.sublistView(response.payload);
             final roomId = respData.getUint32(0, Endian.big);
             // Read room_code (8 bytes)
             final codeBytes = response.payload.sublist(4, 12);
             // Remove null terminators
             final roomCode = utf8.decode(codeBytes.takeWhile((b) => b != 0).toList());
             
             return {"success": true, "roomId": roomId, "roomCode": roomCode};
        } else {
             String? errorMsg;
             try {
                errorMsg = utf8.decode(response.payload);
             } catch(_) {}
             return {"success": false, "error": errorMsg ?? "Server returned error ${response.command}"};
        }
    } catch (e) {
        return {"success": false, "error": e.toString()};
    }
  }

  Future<Room?> joinRoom(int roomId) async {
    return _joinRoomAction(id: roomId);
  }

  Future<Room?> joinRoomByCode(String code) async {
    return _joinRoomAction(code: code);
  }

  Future<Room?> _joinRoomAction({int? id, String? code}) async {
    final buffer = ByteData(16);
    if (code != null) {
      buffer.setUint8(0, 1); // by_code = 1
      final codeBytes = utf8.encode(code);
      for (int i = 0; i < codeBytes.length && i < 8; i++) {
        buffer.setUint8(8 + i, codeBytes[i]);
      }
    } else {
      buffer.setUint8(0, 0); // by_code = 0
      buffer.setUint32(4, id ?? 0);
    }

    try {
      final response = await client.request(Command.joinRoom, payload: buffer.buffer.asUint8List());
      
      if (response.command == Command.resRoomJoined) {
          final jsonStr = utf8.decode(response.payload);
          final json = jsonDecode(jsonStr);
          
          var rules = json['gameRules'] ?? {};
          var playerList = <RoomMember>[];
          if (json['players'] != null) {
              playerList = (json['players'] as List).map((p) => RoomMember(
                  accountId: p['account_id'],
                  email: p['name'] ?? "Unknown",
                  avatar: p['avatar'],
                  ready: p['is_ready'] == true
              )).toList();
          }

          return Room(
              id: json['roomId'],
              name: json['roomName'],
              code: json['roomCode'],
              hostId: json['hostId'],
              maxPlayers: rules['maxPlayers'] ?? 6,
              visibility: rules['visibility'] ?? "public",
              mode: rules['mode'] ?? "scoring",
              wagerMode: rules['wagerMode'] == true,
              roundTime: 15,
              members: playerList,
              status: "waiting",
              currentPlayerCount: playerList.length
          );
      } else if (response.command == Command.errRoomFull) {
           throw "Room is full";
      } else if (response.command == Command.errGameStarted) {
           throw "Game already started";
      } else {
           throw "Failed to join room: ${response.command}";
      }
    } catch (e) {
      print("Error joining room: $e");
      rethrow;
    }
  }

  Future<void> leaveRoom(int roomId) async {
     // Backend handle_leave_room expects empty payload and infers room from session
     await client.request(Command.leaveRoom, payload: Uint8List(0));
  }

  /// Close room (Host only)
  Future<void> closeRoom(int roomId) async {
    final buffer = ByteData(4);
    buffer.setUint32(0, roomId, Endian.big);
    
    print("[RoomService] Closing room $roomId");
    await client.request(Command.closeRoom, payload: buffer.buffer.asUint8List());
  }
  
  Future<void> kickMember(int roomId, int targetId) async {
      final buffer = ByteData(4);
      buffer.setUint32(0, targetId, Endian.big);
      await client.request(Command.kick, payload: buffer.buffer.asUint8List());
  }

  Future<List<Room>> fetchRoomList() async {
    try {
      // CMD_GET_ROOM_LIST takes optional filtering payload, but empty is fine for "all"
      final response = await client.request(Command.cmdGetRoomList, payload: Uint8List(0));
      
      if (response.command == Command.resRoomList) {
          final jsonStr = utf8.decode(response.payload);
          final dynamic json = jsonDecode(jsonStr);
          
          if (json is List) {
             return json.map((r) => Room.fromJson(r)).toList();
          } else if (json is Map && json['success'] == true && json['rooms'] is List) {
              return (json['rooms'] as List).map((r) => Room.fromJson(r)).toList();
          }
      }
      return [];
    } catch (e) {
      print("Error fetching room list: $e");
      return [];
    }
  }
  Future<void> toggleReady() async {
      await client.request(Command.ready);
  }

  Future<void> setRules({
    required String mode,
    required int maxPlayers,
    required String visibility,
    required bool wager,
  }) async {
    final buffer = ByteData(4);
    
    // mode: 0=elimination, 1=scoring
    int modeVal = (mode.toLowerCase() == "elimination") ? 0 : 1;
    buffer.setUint8(0, modeVal);
    
    // max_players
    buffer.setUint8(1, maxPlayers);
    
    // visibility: 0=public, 1=private
    int visibilityVal = (visibility.toLowerCase() == "private") ? 1 : 0;
    buffer.setUint8(2, visibilityVal);
    
    // wager: 0=off, 1=on
    int wagerVal = wager ? 1 : 0;
    buffer.setUint8(3, wagerVal);

    final response = await client.request(Command.setRule, payload: buffer.buffer.asUint8List());
    if (response.command != Command.resRulesUpdated) {
        throw "Failed to update rules: ${response.command}";
    }
  }



  void dispose() {
      _eventController.close();
  }
}
