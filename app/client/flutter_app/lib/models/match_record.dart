import 'dart:typed_data';
import 'dart:convert';

class MatchRecord {
  final int matchId;
  final String mode; // "Scoring" or "Eliminated"
  final bool isWinner;
  final int score;
  final String ranking;
  final DateTime endedAt;
  final bool isForfeit;
  final bool isEliminated;

  MatchRecord({
    required this.matchId,
    required this.mode,
    required this.isWinner,
    required this.score,
    required this.ranking,
    required this.endedAt,
    this.isForfeit = false,
    this.isEliminated = false,
  });

  factory MatchRecord.fromBytes(Uint8List bytes, int offset) {
    final data = ByteData.sublistView(bytes, offset, offset + 32);

    final matchId = data.getUint32(0, Endian.little);
    final modeVal = data.getUint8(4);
    final isWinner = data.getUint8(5) != 0;
    // index 6, 7 are padding
    final score = data.getInt32(8, Endian.little);
    
    // index 12-19 ranking char[8]
    final rankingBytes = bytes.sublist(offset + 12, offset + 20);
    final ranking = utf8.decode(rankingBytes).split('\u0000').first;

    // index 24-31 endedAt int64
    final endedAtTimestamp = data.getInt64(24, Endian.little);
    final endedAt = DateTime.fromMillisecondsSinceEpoch(endedAtTimestamp * 1000);

    String modeStr = "Unknown";
    if (modeVal == 1) modeStr = "Scoring";
    if (modeVal == 2) modeStr = "Eliminated";

    return MatchRecord(
      matchId: matchId,
      mode: modeStr,
      isWinner: isWinner,
      score: score,
      ranking: ranking,
      endedAt: endedAt,
    );
  }
}
