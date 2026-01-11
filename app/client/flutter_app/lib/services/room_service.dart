import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import '../network/tcp_client.dart';
import '../network/dispatcher.dart';
import '../core/command.dart';
import '../models/room.dart';
import '../core/protocol.dart';

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
      // Simple logic to determine host. 
      // For Android Emulator, use 10.0.2.2
      // For others, use localhost. 
      // Note: This relies on Platform.isAndroid which is true on physical devices too.
      // If running on physical device, localhost won't work anyway, need real IP.
      if (Platform.isAndroid) return "http://10.0.2.2:5000/api"; 
      return "http://localhost:5000/api";
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

  Future<Room?> fetchRoom(int roomId) async {
    try {
      final httpClient = HttpClient();
      final request = await httpClient.getUrl(Uri.parse('$_apiBaseUrl/room/$roomId'));
      final response = await request.close();
      
      if (response.statusCode == 200) {
        final stringData = await response.transform(utf8.decoder).join();
        final json = jsonDecode(stringData);
        if (json['success'] == true && json['room'] != null) {
          return Room.fromJson(json['room']);
        }
      }
      return null;
    } catch (e) {
      print("Error fetching room: $e");
      return null;
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

  Future<void> startGame(int roomId) async {
      final buffer = ByteData(4);
      buffer.setUint32(0, roomId);
      await client.request(Command.startGame, payload: buffer.buffer.asUint8List());
  }

  void dispose() {
      _eventController.close();
  }
}
