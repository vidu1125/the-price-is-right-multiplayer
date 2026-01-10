import 'dart:async';
import '../core/command.dart';
import '../core/protocol.dart';
import 'tcp_client.dart';

/// Central router for server-pushed notifications (unsolicited messages)
class Dispatcher {
  final TcpClient client;
  StreamSubscription? _subscription;

  // Handlers for specific notifications
  // Keys are command opcodes (e.g., NTF_PLAYER_JOINED)
  final Map<int, List<Function(DecodedMessage)>> _handlers = {};

  Dispatcher(this.client) {
    _subscription = client.messageStream.listen(_onMessage);
    // Ignore late arriving login responses (handled by request logic usually)
    register(Command.resLoginOk, (msg) {
      print("ℹ️ Late/Unsolicited Login Response: seq=${msg.seq} (Ignored)");
    });
  }

  /// Register a listener for a specific server command/notification
  void register(int command, Function(DecodedMessage) handler) {
    if (!_handlers.containsKey(command)) {
      _handlers[command] = [];
    }
    _handlers[command]!.add(handler);
  }

  void unregister(int command, Function(DecodedMessage) handler) {
    _handlers[command]?.remove(handler);
  }

  void _onMessage(DecodedMessage msg) {
    final commandHandlers = _handlers[msg.command];
    if (commandHandlers != null) {
      for (var handler in commandHandlers) {
        handler(msg);
      }
    } else {
      print("🔔 Unhandled notification: 0x${msg.command.toRadixString(16)}");
    }
  }

  void dispose() {
    _subscription?.cancel();
  }
}
