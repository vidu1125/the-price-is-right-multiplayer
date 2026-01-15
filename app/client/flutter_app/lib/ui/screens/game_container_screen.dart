import 'dart:async';
import 'package:flutter/material.dart';
import '../theme/game_theme.dart';
import '../widgets/round1_widget.dart';
import '../widgets/round2_widget.dart';
import '../widgets/round3_widget.dart';
import '../widgets/game_button.dart';
import '../../services/service_locator.dart';
import '../../services/game_state_service.dart';


class GameContainerScreen extends StatefulWidget {
  const GameContainerScreen({super.key});

  @override
  State<GameContainerScreen> createState() => _GameContainerScreenState();
}

class _GameContainerScreenState extends State<GameContainerScreen> {
  int _matchId = 0;
  int _currentRound = 1;
  Map<String, dynamic>? _currentQuestion;
  List<Map<String, dynamic>> _players = [];
  int _currentScore = 0;
  int _timeRemaining = 60;
  bool _isLoading = true;
  StreamSubscription? _gameEventSub;
  Timer? _countdownTimer;
  bool _initialized = false; // Track if already initialized
  bool _showingResult = false; // Track if currently showing result/feedback

  @override
  void initState() {
    super.initState();
    // Don't call _init() here - will be called in didChangeDependencies
  }

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    // Only initialize once
    if (!_initialized) {
      _initialized = true;
      _init();
    }
  }

  @override
  void dispose() {
    _gameEventSub?.cancel();
    _countdownTimer?.cancel();
    super.dispose();
  }

  Future<void> _init() async {
    // Get arguments from navigation
    final args = ModalRoute.of(context)?.settings.arguments as Map<String, dynamic>? ?? {};
    _matchId = args['matchId'] ?? 0;

    if (_matchId == 0) {
      print("[GameContainer] ERROR: No match_id provided!");
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text("Error: No match ID")),
        );
        Navigator.pop(context);
      }
      return;
    }

    print("[GameContainer] Match ID: $_matchId");

    // Listen to game events
    _gameEventSub = ServiceLocator.gameStateService.events.listen(_handleGameEvent);

    // Send ROUND1_PLAYER_READY to server
    // Backend will automatically send first question when all players are ready
    ServiceLocator.gameStateService.sendRound1Ready(_matchId);
  }

  void _handleGameEvent(GameEvent event) {
    if (!mounted) return;

    setState(() {
      switch (event.type) {
        case GameEventType.roundStart:
          print("[GameContainer] Round started: ${event.data}");
          final data = event.data as Map<String, dynamic>?;
          _currentRound = data?['round'] ?? 1;
          _isLoading = false;
          break;

        case GameEventType.round1Question:
          print("[GameContainer] Question received: ${event.data}");
          final questionData = event.data as Map<String, dynamic>?;
          
          if (_showingResult) {
            // Delay showing next question to let user see result
            Future.delayed(const Duration(seconds: 2), () {
              if (mounted) {
                _applyNextQuestion(questionData);
              }
            });
          } else {
             _applyNextQuestion(questionData);
          }
          break;

        case GameEventType.round1Result:
          print("[GameContainer] Answer result: ${event.data}");
          _showingResult = true; // Flag that we are showing result
          
          final result = event.data as Map<String, dynamic>?;
          final bool isCorrect = result?['is_correct'] == true;
          final int scoreDelta = result?['score_delta'] ?? 0;
          
          if (isCorrect) {
            _currentScore += scoreDelta;
          }
          
          // Update current question with correct index for UI to show
          if (_currentQuestion != null && result?['correct_index'] != null) {
            final newQuestionData = Map<String, dynamic>.from(_currentQuestion!);
            newQuestionData['correctIndex'] = result!['correct_index'];
            _currentQuestion = newQuestionData;
          }
          
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(isCorrect ? "Correct! +$scoreDelta pts" : "Wrong!"),
              backgroundColor: isCorrect ? Colors.green : Colors.red,
              duration: const Duration(seconds: 2),
            ),
          );
          
          // Backend will automatically send next question when all players have answered
          // No need to request manually
          break;


        case GameEventType.round1AllFinished:
          print("[GameContainer] Round 1 finished: ${event.data}");
          final data = event.data as Map<String, dynamic>?;
          
          // Update players/scoreboard
          if (data?['players'] != null) {
            _players = List<Map<String, dynamic>>.from(data!['players']);
          }
          
          // Get next round number
          final nextRound = data?['next_round'] ?? 0;
          print("[GameContainer] Next round: $nextRound");
          
          if (nextRound > 0) {
            // Show round summary briefly then move to next round
            Future.delayed(const Duration(seconds: 3), () {
              if (mounted) {
                setState(() {
                  _currentRound = nextRound;
                  _currentQuestion = null;
                  _isLoading = true;
                });
                
                // Send ready for next round
                if (nextRound == 2) {
                  // Round 2 - Bidding
                  print("[GameContainer] Sending ROUND2_PLAYER_READY");
                  ServiceLocator.gameStateService.sendRound2PlayerReady(_matchId, 0); // 0 = default (server gets from session)
                } else if (nextRound == 3) {
                  // Round 3 - Wheel
                  print("[GameContainer] Sending ROUND3_PLAYER_READY");
                  ServiceLocator.gameStateService.sendRound3PlayerReady(_matchId, 0);
                }
              }
            });
          } else {
            // No more rounds - game finished
            print("[GameContainer] Game completed!");
          }
          break;

        case GameEventType.scoreboard:
          print("[GameContainer] Scoreboard update: ${event.data}");
          final data = event.data as Map<String, dynamic>?;
          if (data?['players'] != null) {
            _players = List<Map<String, dynamic>>.from(data!['players']);
          }
          break;

        case GameEventType.roundEnd:
          print("[GameContainer] Round ended");
          _countdownTimer?.cancel();
          // TODO: Show round summary and move to next round
          break;

        case GameEventType.gameEnd:
          print("[GameContainer] Game ended");
          // TODO: Navigate to results screen
          break;

        case GameEventType.round2Product:
          print("[GameContainer] Product received: ${event.data}");
          _currentQuestion = event.data as Map<String, dynamic>?; // Reusing _currentQuestion map
          _isLoading = false;
          _startCountdown();
          break;

        case GameEventType.round2TurnResult:
           // Show results of the bid (who won closest)
           // TODO: Show nice overlay
           break;

        default:
          break;
      }
    });
  }


  void _startCountdown() {
    _countdownTimer?.cancel();
    _timeRemaining = 15;  // Changed from 60 to 15 seconds
    
    _countdownTimer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (!mounted) {
        timer.cancel();
        return;
      }
      
      setState(() {
        if (_timeRemaining > 0) {
          _timeRemaining--;
        } else {
          timer.cancel();
          // Auto-submit or move to next question
        }
      });
    });
  }


  void _handleAnswerSelected(int choiceIndex) {
    if (_currentQuestion == null) return;
    
    int questionIndex = _currentQuestion?['question_idx'] ?? 0;
    
    print("[GameContainer] Answer selected: question=$questionIndex, choice=$choiceIndex");
    
    ServiceLocator.gameStateService.submitRound1Answer(
      _matchId,
      questionIndex,
      choiceIndex,
    );
    
    _countdownTimer?.cancel();
  }

  @override
  Widget build(BuildContext context) {
    // Get arguments
    final args = ModalRoute.of(context)?.settings.arguments as Map<String, dynamic>? ?? {};
    final int matchId = args['matchId'] ?? _matchId;

    if (_isLoading) {
      return Scaffold(
        body: Container(
          decoration: const BoxDecoration(
            image: DecorationImage(
              image: AssetImage('assets/images/lobby-bg.png'),
              fit: BoxFit.cover,
            ),
          ),
          child: const Center(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                CircularProgressIndicator(color: Color(0xFFFFDE00)),
                SizedBox(height: 20),
                Text(
                  "Loading game...",
                  style: TextStyle(
                    fontFamily: 'LuckiestGuy',
                    fontSize: 24,
                    color: Colors.white,
                  ),
                ),
              ],
            ),
          ),
        ),
      );
    }

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          image: DecorationImage(
            image: AssetImage('assets/images/lobby-bg.png'),
            fit: BoxFit.cover
          ),
        ),
        child: Row(
          children: [
            // LEFT SIDE: MAIN GAME AREA
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
                        RoundIndicatorLarge(roundText: "ROUND $_currentRound"),
                        Row(
                          children: [
                            TimerBlock(seconds: _timeRemaining),
                            const SizedBox(width: 20),
                            ScoreBlock(score: _currentScore),
                          ],
                        ),
                      ],
                    ),
                    const SizedBox(height: 20),
                    // Round content area
                    Expanded(
                      child: Container(
                        padding: const EdgeInsets.all(30),
                        decoration: BoxDecoration(
                          color: const Color(0xB31F2A44),
                          borderRadius: BorderRadius.circular(24),
                          border: Border.all(color: const Color(0xFF1F2A44), width: 4),
                        ),
                        child: _buildRoundWidget(),
                      ),
                    ),
                  ],
                ),
              ),
            ),

            // RIGHT SIDE: RANKING BOX (Sidebar)
            Expanded(
              flex: 1,
              child: Padding(
                padding: const EdgeInsets.fromLTRB(0, 30, 30, 30),
                child: Column(
                  children: [
                    const SizedBox(height: 88),
                    Expanded(
                      child: Container(
                        decoration: BoxDecoration(
                          color: const Color(0xB31F2A44),
                          borderRadius: BorderRadius.circular(24),
                          border: Border.all(color: const Color(0xFF1F2A44), width: 4),
                        ),
                        child: LeaderboardSidebar(
                          matchId: matchId,
                          players: _players,
                        ),
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

  Widget _buildRoundWidget() {
    switch (_currentRound) {
      case 1:
        if (_currentQuestion == null) {
          return const Center(child: Text("Waiting for question...", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 24, color: Colors.white)));
        }
        return Round1Widget(
          questionData: _currentQuestion!,
          onAnswerSelected: _handleAnswerSelected,
        );
      case 2:
        if (_currentQuestion == null) { // Using _currentQuestion to store product data too (or create separate var)
           return const Center(child: Text("Waiting for product...", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 24, color: Colors.white)));
        }
        return Round2Widget(
          productData: _currentQuestion!,
          onSubmitBid: _handleBidSubmitted,
        );
      case 3:
        return const Round3Widget(); // TODO: Implement with real data
      default:
        return const Center(child: Text("Unknown round"));
    }
  }

  void _handleBidSubmitted(int bidAmount) {
     print("[GameContainer] Bid submitted: $bidAmount");
     
     // Get current product index
     int productIdx = _currentQuestion?['product_idx'] ?? 0;
     
     ServiceLocator.gameStateService.submitRound2Bid(
       _matchId,
       productIdx,
       bidAmount
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
        color: const Color(0xFFFFDE00),
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
          fontSize: 32,
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
  final List<Map<String, dynamic>> players;
  
  const LeaderboardSidebar({
    super.key,
    required this.matchId,
    this.players = const [],
  });

  @override
  Widget build(BuildContext context) {
    // Use provided players or fallback to mock data
    final List<Map<String, dynamic>> displayPlayers = players.isNotEmpty
        ? players
        : [
            {'name': 'YOU', 'score': 0, 'isOut': false, 'isMe': true},
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
            itemCount: displayPlayers.length,
            padding: const EdgeInsets.symmetric(horizontal: 16),
            itemBuilder: (context, index) {
              final p = displayPlayers[index];
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
                                p['name'] ?? 'Unknown',
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
                                  '${p['score'] ?? 0} PTS',
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
        // FORFEIT Button
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
                           Navigator.pop(context);
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
