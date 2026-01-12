import 'package:flutter/material.dart';
import '../theme/match_detail_theme.dart';
import '../theme/lobby_theme.dart';
import 'package:google_fonts/google_fonts.dart';

class MatchDetailScreen extends StatelessWidget {
  final Map<String, dynamic> data;

  const MatchDetailScreen({super.key, required this.data});

  @override
  Widget build(BuildContext context) {
    final questions = data['questions'] as List<dynamic>? ?? [];
    
    // Group questions by round_no manually
    final Map<int, List<dynamic>> groupedRounds = {};
    for (var q in questions) {
      final rNo = q['round_no'] as int? ?? 1;
      if (!groupedRounds.containsKey(rNo)) groupedRounds[rNo] = [];
      groupedRounds[rNo]!.add(q);
    }

    final roundNumbers = groupedRounds.keys.toList()..sort();

    // Calculate stats
    final Set<String> uniquePlayers = {};
    for (var q in questions) {
      final answers = q['answers'] as List<dynamic>? ?? [];
      for (var a in answers) {
        if (a['player_name'] != null) uniquePlayers.add(a['player_name']);
      }
    }
    final int playerCount = uniquePlayers.isEmpty ? (data['player_count'] as int? ?? 0) : uniquePlayers.length;
    final String mode = (data['display_mode'] ?? data['mode'] ?? 'SCORING').toString().toUpperCase();
    final String score = (data['score'] ?? '0').toString();
    final String matchId = (data['match_id'] ?? '?').toString();

    return Scaffold(
      extendBodyBehindAppBar: true, 
      body: Stack(
        children: [
          // Background matching History and Lobby
          Positioned.fill(
             child: Image.asset("assets/images/lobby-bg.png", fit: BoxFit.cover),
          ),
          Positioned.fill(child: Container(color: Colors.black.withOpacity(0.5))),
          SafeArea(
            child: Column(
              children: [
                const SizedBox(height: 20),
                _buildSummaryHeader(matchId, mode, score, playerCount),
                const SizedBox(height: 20),
                
                Expanded(
                  child: ListView.builder(
                    padding: const EdgeInsets.symmetric(horizontal: 20),
                    itemCount: roundNumbers.length,
                    itemBuilder: (context, index) {
                      final rNo = roundNumbers[index];
                      return _buildRoundSection(rNo, groupedRounds[rNo]!);
                    },
                  ),
                ),
              ],
            ),
          ),

          // Nút Back
          Positioned(
            top: 40,
            right: 25,
            child: ElevatedButton(
              style: MatchDetailTheme.backHomeButtonStyle,
              onPressed: () => Navigator.pop(context),
              child: Text("BACK", style: LobbyTheme.gameFont(fontSize: 14)),
            ),
          ),
        ],
      ),
    );
  }
  
  Widget _buildSummaryHeader(String matchId, String mode, String score, int players) {
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 20),
      padding: const EdgeInsets.symmetric(vertical: 20, horizontal: 24),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.1),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: MatchDetailTheme.yellowBadge.withOpacity(0.5), width: 1.5),
        boxShadow: [
          BoxShadow(color: Colors.black.withOpacity(0.2), blurRadius: 10, offset: const Offset(0, 5))
        ]
      ),
      child: Column(
        children: [
          Text("MATCH #$matchId", style: GoogleFonts.luckiestGuy(fontSize: 40, color: MatchDetailTheme.yellowBadge, letterSpacing: 2)),
          const SizedBox(height: 20),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _summaryItem("MODE", mode, Colors.lightBlueAccent),
              _summaryItem("SCORING", score, MatchDetailTheme.correctGreen),
              _summaryItem("PLAYERS", "$players", Colors.orangeAccent),
            ],
          ),
        ],
      ),
    );
  }

  Widget _summaryItem(String label, String value, Color color) {
    return Column(
      children: [
        Text(value, style: GoogleFonts.luckiestGuy(fontSize: 36, color: color)),
        const SizedBox(height: 5),
        Text(label, style: GoogleFonts.luckiestGuy(fontSize: 16, color: Colors.white70)),
      ],
    );
  }

  Widget _buildRoundSection(int roundNumber, List<dynamic> questions) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.only(top: 30, bottom: 15),
          child: Stack( // Sử dụng Stack để tạo hiệu ứng đổ bóng/viền cho chữ Round
            children: [
               // Shadow/Outline effect
               Text(
                "ROUND $roundNumber",
                style: LobbyTheme.gameFont(
                  fontSize: 36, 
                  color: Colors.transparent, // Transparent because it's just shadow source
                ).copyWith(
                  shadows: [
                    const Shadow(color: Colors.black, blurRadius: 8, offset: Offset(2, 2)),
                    const Shadow(color: Colors.black, blurRadius: 8, offset: Offset(-2, 2)),
                  ]
                ),
              ),
              // Main Text
              Text(
                "ROUND $roundNumber",
                style: LobbyTheme.gameFont(
                  fontSize: 36, // Tăng size lên hẳn (từ 24 lên 36)
                  color: MatchDetailTheme.yellowBadge, // Chuyển sang màu vàng cho tông xuyệt tông
                ),
              ),
            ],
          ),
        ),
        // Danh sách câu hỏi
        ...questions.map((q) => _buildQuestionCard(q)),
      ],
    );
  }

  Widget _buildQuestionCard(Map<String, dynamic> q) {
    final answers = q['answers'] as List<dynamic>? ?? [];

    return Container(
      margin: const EdgeInsets.only(bottom: 25), // Tăng khoảng cách giữa các card
      padding: const EdgeInsets.all(20), // Tăng padding bên trong
      decoration: MatchDetailTheme.questionCardDecoration,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                decoration: BoxDecoration(
                  color: MatchDetailTheme.yellowBadge,
                  borderRadius: BorderRadius.circular(8),
                ),
                child: const Text("QUESTION", style: MatchDetailTheme.roundBadgeStyle),
              ),
              const SizedBox(width: 10),
              Text("#${q['question_idx'] ?? '?'}", style: LobbyTheme.gameFont(fontSize: 20, color: MatchDetailTheme.yellowBadge)),
            ],
          ),
          const SizedBox(height: 12),
          Text(
            q['question_text'] ?? "No question text availble",
            style: MatchDetailTheme.questionTextStyle,
          ),
          const SizedBox(height: 15),
          const Divider(color: Colors.white10),
          // Bảng câu trả lời của người chơi
          ...answers.map((ans) => _buildPlayerRow(
            ans['player_name'] ?? '?', 
            ans['answer']?.toString() ?? '-', 
            ans['is_correct'] == true, 
            ans['score_delta'] != null ? (ans['score_delta'] >= 0 ? "+${ans['score_delta']}" : "${ans['score_delta']}") : "0"
          )),
        ],
      ),
    );
  }

  Widget _buildPlayerRow(String name, String ans, bool isCorrect, String pts) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 6),
      child: Row(
        children: [
          SizedBox(
            width: 100,
            child: Text(name, style: MatchDetailTheme.playerNameStyle, overflow: TextOverflow.ellipsis),
          ),
          Expanded(
            child: Text(
              ans,
              style: TextStyle(
                fontFamily: 'Parkinsans',
                fontSize: 16,
                fontWeight: FontWeight.bold,
                color: isCorrect ? MatchDetailTheme.correctGreen : MatchDetailTheme.wrongRed,
              ),
            ),
          ),
          Text(pts, 
            style: TextStyle(
              color: pts.startsWith('+') ? Colors.blueAccent : Colors.orangeAccent,
              fontWeight: FontWeight.bold
            ),
          ),
        ],
      ),
    );
  }
}