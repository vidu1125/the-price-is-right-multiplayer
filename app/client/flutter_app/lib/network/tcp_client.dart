import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/foundation.dart';
import '../core/protocol.dart';

enum ConnectionStatus { disconnected, connecting, connected, error }

class TcpClient {
  Socket? _socket;
  int _seq = 1;

  // Pending requests: seqNum -> Completer
  final Map<int, Completer<DecodedMessage>> _pending = {};

  // Unsolicited messages (Notifications)
  final _messageController = StreamController<DecodedMessage>.broadcast();
  Stream<DecodedMessage> get messageStream => _messageController.stream;

  // Connection Status
  final _statusController = StreamController<ConnectionStatus>.broadcast();
  Stream<ConnectionStatus> get statusStream => _statusController.stream;
  
  ConnectionStatus _status = ConnectionStatus.disconnected;
  ConnectionStatus get status => _status;

  // Global listener for auth errors (401)
  VoidCallback? onUnauthenticated;

  Future<void> connect(String host, int port) async {
    if (_status == ConnectionStatus.connected) return;

    _status = ConnectionStatus.connecting;
    _statusController.add(_status);

    try {
      _socket = await Socket.connect(host, port, timeout: const Duration(seconds: 5));
      _status = ConnectionStatus.connected;
      _statusController.add(_status);
      print("🌐 Connected to $host:$port");

      _socket!.listen(
        _handleIncomingData,
        onDone: _handleDisconnect,
        onError: (e) => _handleError("Socket error: $e"),
        cancelOnError: false,
      );
    } catch (e) {
      _handleError("Connection failed: $e");
      rethrow;
    }
  }

  void _handleIncomingData(Uint8List data) {
    try {
      // Logic for splitting packets if multiple come at once could be added here
      // For now, we assume simple packet structure
      final msg = Protocol.decode(data);
      print("[$hashCode] ⬅️ Received: cmd=0x${msg.command.toRadixString(16)}, seq=${msg.seq}");

      // Handle 401 Unauthorized globally (usually command/opcode 401 in this protocol)
      if (msg.command == 401) {
        onUnauthenticated?.call();
      }

      if (_pending.containsKey(msg.seq)) {
        _pending.remove(msg.seq)!.complete(msg);
      } else {
        _messageController.add(msg);
      }
    } catch (e) {
      print("❌ Protocol violation or decode error: $e");
    }
  }

  void _handleDisconnect() {
    print("❌ Disconnected from server");
    _status = ConnectionStatus.disconnected;
    _statusController.add(_status);
    _socket = null;
    
    // Fail all pending requests
    for (var completer in _pending.values) {
      completer.completeError(Exception("Disconnected from server"));
    }
    _pending.clear();
  }

  void _handleError(String err) {
    print("❌ $err");
    _status = ConnectionStatus.error;
    _statusController.add(_status);
    _socket?.destroy();
    _socket = null;
  }

  /// Sends a raw packet
  void send(Uint8List packet) {
    if (_socket == null) {
      print("⚠️ Cannot send: Not connected");
      return;
    }
    _socket!.add(packet);
    _socket!.flush();
  }

  /// Sends a request and waits for a response with the same sequence number
  Future<DecodedMessage> request(int command, {Uint8List? payload}) {
    if (_status != ConnectionStatus.connected) {
      return Future.error(Exception("Not connected to server"));
    }

    final seqNum = _seq++;
    final packet = Protocol.buildPacket(
      command: command,
      seqNum: seqNum,
      payload: payload,
    );

    final completer = Completer<DecodedMessage>();
    _pending[seqNum] = completer;

    send(packet);

    return completer.future.timeout(
      const Duration(seconds: 10),
      onTimeout: () {
        _pending.remove(seqNum);
        throw TimeoutException("Request 0x${command.toRadixString(16)} (seq=$seqNum) timed out");
      },
    );
  }

  /// Fire-and-forget command
  void sendCommand({required int command, Map<String, dynamic>? json}) {
    final payload = json != null ? Protocol.jsonPayload(json) : null;
    final packet = Protocol.buildPacket(
      command: command,
      seqNum: _seq++,
      payload: payload,
    );
    send(packet);
  }

  void dispose() {
    _socket?.destroy();
    _messageController.close();
    _statusController.close();
  }
}
