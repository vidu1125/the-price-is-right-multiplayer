import 'dart:async';
import 'dart:typed_data';
import '../network/tcp_client.dart';
import '../network/dispatcher.dart';
import '../core/command.dart';
import '../core/protocol.dart';

class GameService {
  final TcpClient client;
  final Dispatcher dispatcher;

  GameService(this.client, this.dispatcher);

  /// Start a match in the specified room
  /// Backend will broadcast NTF_GAME_START to all players (no direct response)
  Future<Map<String, dynamic>> startGame(int roomId) async {
    final buffer = ByteData(4);
    buffer.setUint32(0, roomId, Endian.big);

    try {
      // Send command without waiting for response
      // Backend will broadcast NTF_GAME_START to all room members
      final packet = Protocol.buildPacket(
        command: Command.startGame,
        seqNum: 0,
        payload: buffer.buffer.asUint8List(),
      );
      
      client.send(packet);
      
      print("[GameService] Start game request sent for room $roomId");
      
      // Success means command was sent, actual game start is confirmed via NTF_GAME_START
      return {"success": true};
    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  Future<Map<String, dynamic>> forfeit(int matchId) async {
    final buffer = ByteData(4);
    buffer.setUint32(0, matchId);

    try {
      final response = await client.request(Command.forfeit, payload: buffer.buffer.asUint8List());
      if (response.command == Command.resSuccess) {
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
