// lib/core/protocol.dart
import 'dart:convert';
import 'dart:typed_data';

class Protocol {
  static const int magic = 0x4347;
  static const int version = 0x01;
  static const int headerSize = 16;

  // ======================
  // BUILD (đã có)
  // ======================
  static Uint8List buildPacket({
    required int command,
    int flags = 0,
    int seqNum = 0,
    Uint8List? payload,
  }) {
    final len = payload?.length ?? 0;
    final data = ByteData(headerSize + len);

    data.setUint16(0, magic, Endian.big);
    data.setUint8(2, version);
    data.setUint8(3, flags);
    data.setUint16(4, command, Endian.big);
    data.setUint16(6, 0, Endian.big);
    data.setUint32(8, seqNum, Endian.big);
    data.setUint32(12, len, Endian.big);

    if (len > 0) {
      data.buffer.asUint8List().setRange(
        headerSize,
        headerSize + len,
        payload!,
      );
    }

    return data.buffer.asUint8List();
  }

  // ======================
  // DECODE (THÊM)
  // ======================
  static DecodedMessage decode(Uint8List bytes) {
    final data = ByteData.sublistView(bytes);

    final magic = data.getUint16(0, Endian.big);
    final version = data.getUint8(2);
    final command = data.getUint16(4, Endian.big);
    final seq = data.getUint32(8, Endian.big);
    final length = data.getUint32(12, Endian.big);

    if (magic != Protocol.magic || version != Protocol.version) {
      throw Exception("Invalid protocol");
    }

    final payload = length > 0
        ? bytes.sublist(headerSize, headerSize + length)
        : Uint8List(0);

    return DecodedMessage(command, seq, payload);
  }

  static Map<String, dynamic> decodeJson(Uint8List payload) {
    if (payload.isEmpty) return {};
    return jsonDecode(utf8.decode(payload));
  }

  static Uint8List jsonPayload(Map<String, dynamic> obj) {
    return Uint8List.fromList(utf8.encode(jsonEncode(obj)));
  }
}

// ======================
// INTERNAL CLASS (GỘP LUÔN, KHÔNG TÁCH FILE)
// ======================
class DecodedMessage {
  final int command;
  final int seq;
  final Uint8List payload;

  DecodedMessage(this.command, this.seq, this.payload);
}
