import 'package:flutter/material.dart';
import '../theme/match_detail_theme.dart';
import '../theme/lobby_theme.dart';

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

    return Scaffold(
      backgroundColor: MatchDetailTheme.backgroundDark,
      body: Stack(
        children: [
          // Nút Back (Top Right giống Web)
          Positioned(
            top: 40,
            right: 25,
            child: ElevatedButton(
              style: MatchDetailTheme.backHomeButtonStyle,
              onPressed: () => Navigator.pop(context),
              child: Text("BACK", style: LobbyTheme.gameFont(fontSize: 14)),
            ),
          ),

          SafeArea(
            child: Column(
              children: [
                const SizedBox(height: 20),
                Text("MATCH #${data['match_id'] ?? '?'}", 
                  style: LobbyTheme.gameFont(fontSize: 28, color: MatchDetailTheme.yellowBadge)),
                const SizedBox(height: 5),
                Text("MODE: ${data['mode']?.toString().toUpperCase() ?? '?'}", 
                  style: const TextStyle(color: Colors.white54, letterSpacing: 1.5, fontSize: 12)),
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
        ],
      ),
    );
  }

  Widget _buildRoundSection(int roundNumber, List<dynamic> questions) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.symmetric(vertical: 15),
          child: Text("ROUND $roundNumber", 
            style: LobbyTheme.gameFont(fontSize: 20, color: Colors.white70)),
        ),
        // Danh sách câu hỏi trong Round
        ...questions.map((q) => _buildQuestionCard(q)),
      ],
    );
  }

  Widget _buildQuestionCard(Map<String, dynamic> q) {
    final answers = q['answers'] as List<dynamic>? ?? [];

    return Container(
      margin: const EdgeInsets.only(bottom: 15),
      padding: const EdgeInsets.all(16),
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
              Text("#${q['question_idx'] ?? '?'}", style: LobbyTheme.gameFont(fontSize: 16, color: MatchDetailTheme.yellowBadge)),
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