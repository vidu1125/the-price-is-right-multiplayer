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
                Padding(
                  padding: const EdgeInsets.only(top: 10, right: 20, bottom: 10),
                  child: Align(
                    alignment: Alignment.centerRight,
                    child: ElevatedButton(
                      style: MatchDetailTheme.backHomeButtonStyle,
                      onPressed: () => Navigator.pop(context),
                      child: Text("BACK", style: LobbyTheme.gameFont(fontSize: 20)), // Increased size
                    ),
                  ),
                ),
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
          padding: const EdgeInsets.only(top: 40, bottom: 20),
          child: Text(
            "ROUND $roundNumber",
            style: GoogleFonts.luckiestGuy(
              fontSize: 42, // Tăng kích thước cực đại
              color: MatchDetailTheme.yellowBadge,
              letterSpacing: 3,
              shadows: [
                const Shadow(color: Colors.black54, offset: Offset(3, 3), blurRadius: 5),
              ],
            ),
          ),
        ),
        ...questions.map((q) => _buildQuestionCard(q)),
      ],
    );
  }

  Widget _buildQuestionCard(Map<String, dynamic> q) {
    final answers = q['answers'] as List<dynamic>? ?? [];

    return Container(
      margin: const EdgeInsets.only(bottom: 25),
      padding: const EdgeInsets.all(24),
      decoration: MatchDetailTheme.questionCardDecoration,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text("QUESTION #${q['question_idx'] ?? '?'}", 
                style: GoogleFonts.luckiestGuy(color: MatchDetailTheme.primaryBlue, fontSize: 18)),
              const Icon(Icons.help_outline, color: MatchDetailTheme.primaryBlue),
            ],
          ),
          const Divider(height: 30, thickness: 1),
          Text(q['question_text'] ?? "", style: MatchDetailTheme.questionTextStyle),
          const SizedBox(height: 20),
          // Danh sách người chơi với màu sắc tương phản cao
          ...answers.map((ans) => _buildPlayerRow(ans)),
        ],
      ),
    );
  }

  Widget _buildPlayerRow(Map<String, dynamic> data) {
    final String name = data['player_name'] ?? '?';
    final String ans = data['answer']?.toString() ?? '-';
    final bool isCorrect = data['is_correct'] == true;
    final int scoreDelta = data['score_delta'] ?? 0;
    final String pts = scoreDelta >= 0 ? "+$scoreDelta" : "$scoreDelta";
    
    // Status check
    final String status = (data['status'] ?? '').toString().toLowerCase();
    final bool isEliminated = status == 'eliminated' || data['is_eliminated'] == true;
    final bool isForfeited = status == 'forfeit' || status == 'forfeited' || data['is_forfeit'] == true;

    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 6),
      child: Row(
        children: [
          SizedBox(
            width: 100,
            child: Text(name, style: MatchDetailTheme.playerNameStyle, overflow: TextOverflow.ellipsis),
          ),
          Expanded(
            child: Row(
              children: [
                Text(
                  ans,
                  style: TextStyle(
                    fontFamily: 'Parkinsans',
                    fontSize: 20,
                    fontWeight: FontWeight.bold,
                    color: isCorrect ? MatchDetailTheme.correctGreen : MatchDetailTheme.wrongRed,
                  ),
                ),
                if (isEliminated) _buildStatusBadge("ELIMINATED", Colors.orange),
                if (isForfeited) _buildStatusBadge("FORFEITED", Colors.red),
              ],
            ),
          ),
          Text(pts, 
            style: TextStyle(
              color: pts.startsWith('+') ? Colors.blueAccent : Colors.orangeAccent,
              fontSize: 20,
              fontWeight: FontWeight.bold
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildStatusBadge(String text, Color color) {
    return Container(
      margin: const EdgeInsets.only(left: 10),
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(color: color, borderRadius: BorderRadius.circular(6)),
      child: Text(text, style: const TextStyle(color: Colors.white, fontSize: 12, fontWeight: FontWeight.bold)),
    );
  }
}