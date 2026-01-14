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
  rulesChanged
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
    
    dispatcher.register(Command.ntfGameStart, (msg) {
        var json = Protocol.decodeJson(msg.payload);
        _eventController.add(RoomEvent(RoomEventType.gameStarted, json));
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
             return {"success": true, "roomId": roomId};
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
    final buffer = ByteData(16);
    buffer.setUint8(0, 0); // by_code = 0 (join by id)
    // padding 3 bytes
    buffer.setUint32(4, roomId);
    // room_code 8 bytes (padded with 0s)

    try {
      final response = await client.request(Command.joinRoom, payload: buffer.buffer.asUint8List());
      
      if (response.command == Command.resRoomJoined) {
          final jsonStr = utf8.decode(response.payload);
          final json = jsonDecode(jsonStr);
          // JSON response from handle_join_room contains:
          // roomId, roomCode, roomName, hostId, isHost, gameRules (mode, maxPlayers, visibility, wagerMode), players (list)
          
          // We need to map this to our Room model.
          // Note context: Room.fromJson matches the standard consistent JSON structure.
          // Check backend JSON construction in handle_join_room:
          // "roomId", "roomCode", "roomName", "hostId", "gameRules": {...}, "players": [...]
          
          // Room.fromJson expects: id, name, code, host_id, max_players, visibility...
          // The keys might differ slighty!
          // handle_join_room keys: roomId, roomCode, roomName, hostId
          // Room.fromJson keys: id, name, code, host_id
          
          // We must adapt the JSON or update Room.fromJson. 
          // Updating Room.fromJson is better for consistency if we can standartize.
          // But Room.fromJson is likely built for "GET_ROOM_LIST" which uses SQL column names (snake_case).
          
          // Let's manually map here to be safe and quick.
          
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
              roundTime: 15, // Default for now
              members: playerList,
              status: "waiting", // Implicit
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
     final buffer = ByteData(4);
     buffer.setUint32(0, roomId);
     await client.request(Command.leaveRoom, payload: buffer.buffer.asUint8List());
  }
  
  Future<void> kickMember(int roomId, int targetId) async {
      final buffer = ByteData(8);
      buffer.setUint32(0, roomId);
      buffer.setUint32(4, targetId);
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



  void dispose() {
      _eventController.close();
  }
}
