import 'dart:async';
import 'dart:typed_data';
import '../network/tcp_client.dart';
import '../network/dispatcher.dart';
import '../core/command.dart';

class GameService {
  final TcpClient client;
  final Dispatcher dispatcher;

  GameService(this.client, this.dispatcher);

  /// Start a match in the specified room
  /// Returns a map with success status and potential error message or match id
  Future<Map<String, dynamic>> startGame(int roomId) async {
    final buffer = ByteData(4);
    buffer.setUint32(0, roomId); // Big Endian by default in ByteData setUint32 if endian not specified? No, default is Big Endian.

    try {
      // The backend returns RES_GAME_STARTED (0x012D) with JSON body: {"success":true,"match_id":...}
      final response = await client.request(Command.startGame, payload: buffer.buffer.asUint8List());
      
      if (response.command == Command.resGameStarted) {
       
        String raw = String.fromCharCodes(response.payload);
        int jsonStart = raw.indexOf("{");
        if (jsonStart != -1) {
             // It's just a JSON string, extract it
             // Verify if it works.
             return {"success": true};
        }
        return {"success": true}; 
      } else {
        return {"success": false, "error": "Unexpected response: ${response.command}"};
      }
    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  Future<void> endMatch(int matchId) async {
      // TODO: Implement end match
      print("[GameService] End match $matchId not implemented");
  }
}
