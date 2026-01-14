import 'package:flutter/material.dart';
import '../theme/game_theme.dart';
import '../widgets/round1_widget.dart';
import '../widgets/round2_widget.dart';
import '../widgets/round3_widget.dart';
import '../widgets/game_button.dart';
import '../../services/service_locator.dart';


class GameContainerScreen extends StatelessWidget {
  const GameContainerScreen({super.key});

  @override
  Widget build(BuildContext context) {
    // Get arguments
    final args = ModalRoute.of(context)?.settings.arguments as Map<String, dynamic>? ?? {};
    final int matchId = args['matchId'] ?? 1;

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          image: DecorationImage(
            image: AssetImage('assets/images/lobby-bg.png'), // Background
            fit: BoxFit.cover
          ),
        ),
        child: Row(
          children: [
            // BÊN TRÁI: KHU VỰC CHƠI CHÍNH
            Expanded(
              flex: 3,
              child: Padding(
                padding: const EdgeInsets.all(30.0),
                child: Column(
                  children: [
                    // Header Row: ROUND on left, Time & Score on right
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      crossAxisAlignment: CrossAxisAlignment.end,
                      children: [
                        const RoundIndicatorLarge(roundText: "ROUND 1"),
                        Row(
                          children: [
                            const TimerBlock(seconds: 60),
                            const SizedBox(width: 20),
                            const ScoreBlock(score: 1250),
                          ],
                        ),
                      ],
                    ),
                    const SizedBox(height: 20),
                    // Khu vực nội dung Round
                    Expanded(
                      child: Container(
                        padding: const EdgeInsets.all(30),
                        decoration: BoxDecoration(
                          color: const Color(0xB31F2A44), // Navy Transparent (matching room list)
                          borderRadius: BorderRadius.circular(24),
                          border: Border.all(color: const Color(0xFF1F2A44), width: 4),
                        ),
                        child: Round1Widget(
                          questionData: const {
                            'index': 1,
                            'question': 'How much do you think this legendary rare item costs in the current collector market?',
                            'choices': ['\$1,200', '\$2,500', '\$4,800', '\$10,000'],
                            'correctIndex': 1
                          },
                          onAnswerSelected: (index) => print("Answer selected: $index"),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),


            // BÊN PHẢI: RANKING BOX (Sidebar)
            Expanded(
              flex: 1,
              child: Padding(
                padding: const EdgeInsets.fromLTRB(0, 30, 30, 30),
                child: Column(
                  children: [
                    // Spacer to align with ROUND 1 indicator height
                    const SizedBox(height: 88), // Fine-tuned to match question area top alignment
                    // Ranking box aligned with question area
                    Expanded(
                      child: Container(
                        decoration: BoxDecoration(
                          color: const Color(0xB31F2A44), // Navy Transparent (matching question area)
                          borderRadius: BorderRadius.circular(24),
                          border: Border.all(color: const Color(0xFF1F2A44), width: 4),
                        ),
                        child: LeaderboardSidebar(matchId: matchId),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}


// --- Widgets ---

// Timer Block Widget
class TimerBlock extends StatelessWidget {
  final int seconds;
  const TimerBlock({super.key, required this.seconds});

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 100,
      height: 100,
      decoration: BoxDecoration(
        color: const Color(0xFFFF5E5E),
        shape: BoxShape.circle,
        border: Border.all(color: const Color(0xFF2D3436), width: 5),
        boxShadow: const [
          BoxShadow(offset: Offset(6, 6), color: Colors.black26),
        ],
      ),
      alignment: Alignment.center,
      child: Text(
        "$seconds",
        style: const TextStyle(
          fontFamily: 'LuckiestGuy',
          fontSize: 42,
          color: Colors.white,
        ),
      ),
    );
  }
}

// Score Block Widget
class ScoreBlock extends StatelessWidget {
  final int score;
  const ScoreBlock({super.key, required this.score});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 15),
      decoration: BoxDecoration(
        color: const Color(0xFF4CAF50),
        borderRadius: BorderRadius.circular(24),
        border: Border.all(color: const Color(0xFF2D3436), width: 5),
        boxShadow: const [
          BoxShadow(offset: Offset(6, 6), color: Colors.black26),
        ],
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          const Icon(Icons.stars, color: Color(0xFFFFD700), size: 40),
          const SizedBox(width: 16),
          Text(
            "$score",
            style: const TextStyle(
              fontFamily: 'LuckiestGuy',
              fontSize: 42,
              color: Colors.white,
            ),
          ),
        ],
      ),
    );
  }
}

// Round Indicator Widget
class RoundIndicatorLarge extends StatelessWidget {
  final String roundText;
  const RoundIndicatorLarge({super.key, required this.roundText});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 15),
      decoration: BoxDecoration(
        color: const Color(0xFFFFDE00), // Màu vàng đặc trưng
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: const Color(0xFF2D3436), width: 5),
        boxShadow: const [
          BoxShadow(offset: Offset(6, 6), color: Colors.black26),
        ],
      ),
      child: Text(
        roundText,
        style: const TextStyle(
          fontFamily: 'LuckiestGuy',
          fontSize: 32, // Phóng to font
          color: Color(0xFF2D3436),
          letterSpacing: 2,
        ),
      ),
    );
  }
}

// Leaderboard Sidebar Widget

class LeaderboardSidebar extends StatelessWidget {
  final int matchId;
  const LeaderboardSidebar({super.key, required this.matchId});

  @override
  Widget build(BuildContext context) {
    final List<Map<String, dynamic>> players = [
      {'name': 'ALEX_MASTER', 'score': 2100, 'isOut': true, 'isMe': false},
      {'name': 'YOU', 'score': 1250, 'isOut': false, 'isMe': true},
      {'name': 'CHRIS_99', 'score': 950, 'isOut': false, 'isMe': false},
    ];

    return Column(
      children: [
        const Padding(
          padding: EdgeInsets.symmetric(vertical: 24),
          child: Text(
            "RANKING",
            style: TextStyle(
              fontFamily: 'LuckiestGuy',
              fontSize: 42,
              color: Color(0xFFFFDE00),
              shadows: [
                Shadow(offset: Offset(4, 4), color: Colors.black45),
              ],
            ),
          ),
        ),
        Expanded(
          child: ListView.builder(
            itemCount: players.length,
            padding: const EdgeInsets.symmetric(horizontal: 16),
            itemBuilder: (context, index) {
              final p = players[index];
              final rank = index + 1;
              final bool isMe = p['isMe'] ?? false;
              final bool isOut = p['isOut'] ?? false;
              
              return Opacity(
                opacity: isOut ? 0.6 : 1.0,
                child: Container(
                  margin: const EdgeInsets.only(bottom: 14),
                  decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(20),
                    boxShadow: [
                      BoxShadow(
                        color: Colors.black.withOpacity(0.3),
                        offset: const Offset(4, 4),
                        blurRadius: 0,
                      ),
                    ],
                  ),
                  child: Container(
                    padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 12),
                    decoration: BoxDecoration(
                      color: isMe ? const Color(0xFFFFDE00) : const Color(0xFF2D3436),
                      borderRadius: BorderRadius.circular(16),
                      border: Border.all(
                        color: isMe ? Colors.white : (isOut ? Colors.redAccent : const Color(0xFF1F2A44)),
                        width: 4,
                      ),
                    ),
                    child: Row(
                      children: [
                        // Coin Style Rank Badge
                        Container(
                          width: 46,
                          height: 46,
                          decoration: BoxDecoration(
                            color: isMe ? Colors.white : (rank <= 3 ? const Color(0xFFFFDE00) : Colors.white10),
                            shape: BoxShape.circle,
                            border: Border.all(
                              color: isMe ? const Color(0xFF2D3436) : Colors.white,
                              width: 3,
                            ),
                            boxShadow: [
                              BoxShadow(
                                color: Colors.black.withOpacity(0.2),
                                offset: const Offset(2, 2),
                              )
                            ]
                          ),
                          alignment: Alignment.center,
                          child: Text(
                            '$rank',
                            style: TextStyle(
                              fontFamily: 'LuckiestGuy',
                              fontSize: 22,
                              color: isMe ? const Color(0xFF2D3436) : (rank <= 3 ? Colors.black : Colors.white),
                            ),
                          ),
                        ),
                        const SizedBox(width: 14),
                        // Player Info
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              Text(
                                p['name'],
                                style: TextStyle(
                                  fontFamily: 'LuckiestGuy',
                                  fontSize: 18,
                                  color: isMe ? const Color(0xFF2D3436) : Colors.white,
                                  decoration: isOut ? TextDecoration.lineThrough : null,
                                  decorationColor: Colors.red,
                                  decorationThickness: 2,
                                  letterSpacing: 1,
                                ),
                                overflow: TextOverflow.ellipsis,
                              ),
                              const SizedBox(height: 2),
                              if (!isOut)
                                Text(
                                  '${p['score']} PTS',
                                  style: TextStyle(
                                    fontFamily: 'LuckiestGuy',
                                    fontSize: 18,
                                    color: isMe ? const Color(0xFF2D3436).withOpacity(0.9) : const Color(0xFFFFDE00),
                                    letterSpacing: 1,
                                  ),
                                ),
                            ],
                          ),
                        ),
                        // Status Stamp
                        if (isOut)
                          Container(
                            padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                            decoration: BoxDecoration(
                              color: Colors.redAccent,
                              borderRadius: BorderRadius.circular(8),
                              border: Border.all(color: Colors.white, width: 2),
                              boxShadow: const [BoxShadow(offset: Offset(2, 2), color: Colors.black26)]
                            ),
                            child: const Text(
                              'OUT',
                              style: TextStyle(
                                fontFamily: 'LuckiestGuy',
                                fontSize: 13,
                                color: Colors.white,
                              ),
                            ),
                          ),
                      ],
                    ),
                  ),
                ),
              );
            },
          ),
        ),
        // FORFEIT Button - Centered and Larger
        Padding(
          padding: const EdgeInsets.only(bottom: 30),
          child: Center(
            child: SizedBox(
              width: 250,
              height: 75,
              child: GameButton(
                text: "FORFEIT",
                color: Colors.redAccent,
                onPressed: () async {
                   final res = await ServiceLocator.gameService.forfeit(matchId);
                   if (res['success'] == true) {
                       if (context.mounted) {
                           Navigator.pop(context); // Go back to lobby/room
                       }
                   } else {
                       if (context.mounted) {
                           ScaffoldMessenger.of(context).showSnackBar(
                               SnackBar(content: Text("Forfeit failed: ${res['error'] ?? 'Unknown error'}"))
                           );
                       }
                   }
                },
              ),
            ),
          ),
        )
      ],
    );
  }
}




