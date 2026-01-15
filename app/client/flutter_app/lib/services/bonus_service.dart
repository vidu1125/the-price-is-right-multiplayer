import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import '../network/tcp_client.dart';
import '../network/dispatcher.dart';
import '../core/command.dart';
import '../core/protocol.dart';

enum BonusEventType {
  init,
  participant,
  spectator,
  cardDrawn,
  playerDrew,
  reveal,
  result,
  transition
}

class BonusEvent {
  final BonusEventType type;
  final dynamic data;
  BonusEvent(this.type, [this.data]);
}

class BonusService {
  final TcpClient client;
  final Dispatcher dispatcher;
  final _eventController = StreamController<BonusEvent>.broadcast();

  Stream<BonusEvent> get events => _eventController.stream;

  BonusService(this.client, this.dispatcher) {
    _registerHandlers();
  }

  void _registerHandlers() {
    dispatcher.register(Command.ntfBonusInit, (msg) {
      final json = Protocol.decodeJson(msg.payload);
      _eventController.add(BonusEvent(BonusEventType.init, json));
    });

    dispatcher.register(Command.ntfBonusParticipant, (msg) {
      final json = Protocol.decodeJson(msg.payload);
      _eventController.add(BonusEvent(BonusEventType.participant, json));
    });

    dispatcher.register(Command.ntfBonusSpectator, (msg) {
      final json = Protocol.decodeJson(msg.payload);
      _eventController.add(BonusEvent(BonusEventType.spectator, json));
    });

    dispatcher.register(Command.ntfBonusCardDrawn, (msg) {
      final json = Protocol.decodeJson(msg.payload);
      _eventController.add(BonusEvent(BonusEventType.cardDrawn, json));
    });

    dispatcher.register(Command.ntfBonusPlayerDrew, (msg) {
      final json = Protocol.decodeJson(msg.payload);
      _eventController.add(BonusEvent(BonusEventType.playerDrew, json));
    });

    dispatcher.register(Command.ntfBonusReveal, (msg) {
      final json = Protocol.decodeJson(msg.payload);
      _eventController.add(BonusEvent(BonusEventType.reveal, json));
    });

    dispatcher.register(Command.ntfBonusResult, (msg) {
      final json = Protocol.decodeJson(msg.payload);
      _eventController.add(BonusEvent(BonusEventType.result, json));
    });

    dispatcher.register(Command.ntfBonusTransition, (msg) {
      final json = Protocol.decodeJson(msg.payload);
      _eventController.add(BonusEvent(BonusEventType.transition, json));
    });
  }

  Future<void> sendBonusReady(int matchId) async {
    final payload = ByteData(4)..setUint32(0, matchId, Endian.big);
    final packet = Protocol.buildPacket(
      command: Command.bonusReady,
      seqNum: 0,
      payload: payload.buffer.asUint8List(),
    );
    client.send(packet);
  }

  Future<void> drawCard(int matchId) async {
    final payload = ByteData(4)..setUint32(0, matchId, Endian.big);
    final packet = Protocol.buildPacket(
      command: Command.bonusDrawCard,
      seqNum: 0,
      payload: payload.buffer.asUint8List(),
    );
    client.send(packet);
  }

  void dispose() {
    _eventController.close();
  }
}
