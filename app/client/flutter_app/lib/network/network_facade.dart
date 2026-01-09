import '../core/protocol.dart';
import '../core/command.dart';
import 'tcp_client.dart';

class NetworkFacade {
  final TcpClient client;
  int _seq = 1;

  NetworkFacade(this.client);

  void joinRoom(int roomId) {
    client.send(
      Protocol.buildPacket(
        command: Command.joinRoom,
        seqNum: _seq++,
        payload: Protocol.jsonPayload({"room_id": roomId}),
      ),
    );
  }

  void startMatch() {
    client.send(
      Protocol.buildPacket(
        command: Command.startMatch,
        seqNum: _seq++,
      ),
    );
  }
}
