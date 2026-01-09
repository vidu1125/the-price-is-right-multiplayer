import 'dart:typed_data';
import 'dart:convert';
import '../network/tcp_client.dart';
import '../core/command.dart';
import '../models/match_record.dart';

class HistoryService {
  final TcpClient client;
  List<MatchRecord>? _cachedHistory;

  HistoryService(this.client);

  List<MatchRecord>? get cachedHistory => _cachedHistory;

  Future<List<MatchRecord>> viewHistory({int limit = 10, int offset = 0, bool forceUpdate = false}) async {
    if (!forceUpdate && _cachedHistory != null) {
      return _cachedHistory!;
    }

    final payload = Uint8List(2);
    payload[0] = limit;
    payload[1] = offset;

    try {
      final response = await client.request(Command.history, payload: payload);
      final data = response.payload;

      if (data.length < 2) {
        throw Exception("Payload too short");
      }

      final count = data[0];
      final List<MatchRecord> records = [];
      const recordSize = 32;

      for (int i = 0; i < count; i++) {
        final recordOffset = 2 + (i * recordSize);
        if (recordOffset + recordSize <= data.length) {
          records.add(MatchRecord.fromBytes(data, recordOffset));
        }
      }

      _cachedHistory = records;
      return records;
    } catch (e) {
      print("❌ HistoryService Error: $e");
      rethrow;
    }
  }

  Future<Map<String, dynamic>> viewMatchDetails(int matchId) async {
    final payload = ByteData(4);
    payload.setUint32(0, matchId, Endian.little);

    try {
      final response = await client.request(Command.replay, payload: payload.buffer.asUint8List());
      final jsonString = utf8.decode(response.payload).split('\u0000').first;
      return jsonDecode(jsonString);
    } catch (e) {
      print("❌ HistoryService Detail Error: $e");
      rethrow;
    }
  }
}
