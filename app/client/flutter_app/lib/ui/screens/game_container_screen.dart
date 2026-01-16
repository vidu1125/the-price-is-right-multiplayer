import 'dart:async';
import 'package:flutter/material.dart';
import '../theme/game_theme.dart';
import '../widgets/round1_widget.dart';
import '../widgets/round2_widget.dart';
import '../widgets/round3_widget.dart';
import '../widgets/round_bonus_widget.dart';
import '../widgets/game_button.dart';
import '../../services/service_locator.dart';
import '../../services/game_state_service.dart';
import 'package:google_fonts/google_fonts.dart';


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
  bool _showDecision = false; // Track if showing Stop/Continue decision
  int _round3Score = -999; // Current score in Round 3 (Wheel). -999 means "pending" or "no result".
  int _spinNumber = 1; // Current spin number (1 or 2)
  int? _myPlayerId; // Current user ID
  bool _inBonusRound = false; 
  List<String> _wheelSegments = [];

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
    // Get current user ID
    final authState = await ServiceLocator.authService.getAuthState();
    final accountIdStr = authState['accountId'];
    if (accountIdStr != null) {
      _myPlayerId = int.tryParse(accountIdStr);
    }

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

        case GameEventType.round1ReadyStatus:
        case GameEventType.round2ReadyStatus:
        case GameEventType.round3ReadyStatus:
           final _statusData = event.data as Map<String, dynamic>?;
           if (_statusData?['players'] != null) {
              _updateLeaderboard(_statusData!['players']);
           }
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
             _updateLeaderboard(data!['players']);
          } else if (data?['rankings'] != null) {
             _updateLeaderboard(data!['rankings']);
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
            _updateLeaderboard(data!['players']);
          }
          break;

        case GameEventType.elimination:
          print("[GameContainer] ELIMINATION: ${event.data}");
          final data = event.data as Map<String, dynamic>?;
          if (data?['player_id'] == _myPlayerId) {
             // IT'S ME who got eliminated!
             ScaffoldMessenger.of(context).showSnackBar(
               SnackBar(
                 content: Text("YOU HAVE BEEN ELIMINATED! (Reason: ${data?['reason'] ?? 'Lowest Score'})"),
                 backgroundColor: Colors.red,
                 duration: const Duration(seconds: 4),
               ),
             );
             
             // Clean up subscriptions immediately to prevent receiving stale notifications
             _gameEventSub?.cancel();
             _gameEventSub = null;
             _countdownTimer?.cancel();
                          // Navigate back to room after 2 seconds
              final roomId = data?['room_id'];
              Future.delayed(const Duration(seconds: 2), () async {
                if (mounted) {
                  if (roomId != null) {
                     // Backend handles re-joining if player is already in room (returns room info)
                     final room = await ServiceLocator.roomService.joinRoom(roomId);
                     if (room != null) {
                        Navigator.pushNamedAndRemoveUntil(context, '/room', (route) => false, arguments: room);
                        return;
                     }
                  }
                  // Fallback to lobby
                  Navigator.pushNamedAndRemoveUntil(context, '/lobby', (route) => false);
                }
              });
          } else {
            // Someone else got eliminated, update leaderboard
            if (_players.isNotEmpty) {
              final List<Map<String, dynamic>> updated = List.from(_players);
              for (int i = 0; i < updated.length; i++) {
                if (updated[i]['id'] == data?['player_id']) {
                  setState(() {
                    updated[i]['isOut'] = true;
                    _players = updated;
                  });
                  break;
                }
              }
            }
          }
          break;

        case GameEventType.bonusStart:
          print("[GameContainer] BONUS ROUND STARTING!");
          _inBonusRound = true;
          _isLoading = false;
          _showingResult = false;
          break;

        case GameEventType.roundEnd:
          print("[GameContainer] Round ended");
          _countdownTimer?.cancel();
          break;

        case GameEventType.gameEnd:
          print("[GameContainer] Game ended: ${event.data}");
          final data = event.data as Map<String, dynamic>?;
          final roomId = data?['room_id'];
          
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
               content: Text("MATCH FINISHED! Returning to room in 5 seconds..."),
               duration: Duration(seconds: 5),
            )
          );

          Future.delayed(const Duration(seconds: 5), () async {
            if (mounted) {
               if (roomId != null) {
                  final room = await ServiceLocator.roomService.joinRoom(roomId);
                  if (room != null) {
                     Navigator.pushNamedAndRemoveUntil(context, '/room', (route) => false, arguments: room);
                     return;
                  }
               }
               Navigator.pushNamedAndRemoveUntil(context, '/lobby', (route) => false);
            }
          });
          break;

        case GameEventType.round2Product:
          print("[GameContainer] Product received: ${event.data}");
          _currentQuestion = event.data as Map<String, dynamic>?; // Reusing _currentQuestion map
          _isLoading = false;
          _startCountdown();
          break;

        case GameEventType.round2TurnResult:
           print("[GameContainer] Turn result: ${event.data}");
           final result = event.data as Map<String, dynamic>?;
           final int actualPrice = result?['correct_price'] ?? 0;
           final List bids = result?['bids'] ?? [];
           
           // Update leaderboard in real-time
           _updatePlayerScores(bids);
           
           int maxScore = 0;
           String winnerName = "None";
           int myScoreDelta = 0;
           
           for (var b in bids) {
              int score = b['score_delta'] ?? 0;
              int id = b['id'] ?? 0;
              
              if (score > maxScore) {
                maxScore = score;
                winnerName = b['name'] ?? "Player";
              }
              
              if (_myPlayerId != null && id == _myPlayerId) {
                 myScoreDelta = score;
              }
           }
           
           if (myScoreDelta > 0) {
              setState(() {
                _currentScore += myScoreDelta;
              });
           }

           ScaffoldMessenger.of(context).showSnackBar(
             SnackBar(
               content: Text("Price: $actualPrice | Winner: $winnerName (+$maxScore)"),
               duration: const Duration(seconds: 3),
               backgroundColor: myScoreDelta > 0 ? Colors.green : (maxScore > 0 ? Colors.orange : Colors.grey),
             ),
           );
           _showingResult = true;
           break;

  // Helper to update player scores and sort leaderboard
  void _updatePlayerScores(List<dynamic> updates) {
    if (_players.isEmpty) return;
    
    bool changed = false;
    for (var update in updates) {
       // Support both id formats
       int id = update['id'] ?? update['account_id'] ?? 0;
       if (id == 0 && update['is_me'] == true && _myPlayerId != null) {
          id = _myPlayerId!;
       }
       if (id == 0) continue;

       // Find player
       int index = _players.indexWhere((p) => p['id'] == id || p['account_id'] == id);
       if (index != -1) {
          // Determine new score
          int? newScore = update['total_score'] ?? update['current_score'];
          
          // Calculate from delta if total not provided
          if (newScore == null && update['score_delta'] != null && update['score_delta'] is int) {
             int current = _players[index]['score'] ?? 0;
             newScore = current + (update['score_delta'] as int);
          }
          
          if (newScore != null) {
              _players[index]['score'] = newScore;
              changed = true;
          }
       }
    }
    
    if (changed) {
       setState(() {
          // Sort by score descending
          _players.sort((a, b) {
             int scoreA = a['score'] ?? 0;
             int scoreB = b['score'] ?? 0;
             return scoreB.compareTo(scoreA); 
          });
       });
    }
  }

        case GameEventType.round2AllFinished:
          print("[GameContainer] Round 2 finished event received: ${event.data}");
          final data = event.data as Map<String, dynamic>?;
          
          // Update players/scoreboard
          if (data?['players'] != null) {
             print("[GameContainer] Updating scoreboard");
            _updateLeaderboard(data!['players']);
          } else if (data?['rankings'] != null) {
            _updateLeaderboard(data!['rankings']);
          }
          
          // Get next round number
          int nextRound = data?['next_round'] ?? 0;
          print("[GameContainer] Next round from payload: $nextRound");
          
          // Fallback if next_round is missing but we know round 3 follows round 2
          if (nextRound == 0) {
             print("[GameContainer] Next round missing, defaulting to 3");
             nextRound = 3;
          }
          
          if (nextRound > 0) {
            // Show round summary briefly then move to next round
            Future.delayed(const Duration(seconds: 3), () {
              if (mounted) {
                print("[GameContainer] Switching to round $nextRound");
                setState(() {
                  _currentRound = nextRound;
                  _currentQuestion = null;
                  _isLoading = true;
                });
                
                // Send ready for next round
                if (nextRound == 3) {
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


        case GameEventType.round3AllReady:
          print("[GameContainer] Round 3 all ready: ${event.data}");
          final data = event.data as Map<String, dynamic>?;
          
          // NOTE: We ignore 'segments' from JSON here because Round3Widget uses hardcoded segments
          // to exactly match the visual design requirements (Mario theme colors/ordering).
          
          if (_currentRound == 3) {
             setState(() => _isLoading = false);
          }
          break;

        case GameEventType.round3DecisionPrompt:
           print("[GameContainer] Round 3 prompt: ${event.data}");
           final data = event.data as Map<String, dynamic>?;
           String message = data?['message'] ?? "";
           
           // Only show decision buttons if the message explicitly asks for a decision (Stop/Continue)
           // and not just a "Spin the wheel" or "Spin again" instruction.
           if (message.contains("Continue or Stop?")) {
             setState(() {
                _showDecision = true;
                _isLoading = false;
             });
            } else if (message.contains("Spin the wheel!")) {
              // Initial round start
              setState(() {
                _isLoading = false;
                _showDecision = false;
                _round3Score = -999; // Reset to "Pending" so it doesn't auto-spin
                _spinNumber = 1;
              });
            } else if (message.contains("Spin again!")) {
              setState(() {
                _spinNumber = 2;
                _round3Score = -999; // Reset to "Pending"
                _showDecision = false;
              });
            }
           break;

        case GameEventType.round3SpinResult:
           print("[GameContainer] Spin result: ${event.data}");
           final data = event.data as Map<String, dynamic>?;
           if (data != null) {
              int spinNumber = data['spin_number'] ?? 1;
              int currentResult = data['result'] ?? 0;
              bool decisionPending = data['decision_pending'] ?? false;
              
              setState(() {
                _round3Score = currentResult; // Show current spin result on wheel
                _spinNumber = spinNumber;
              });
              
              if (decisionPending) {
                // Wait for player decision after spin 1
                setState(() => _showDecision = true);
              } else {
                setState(() => _showDecision = false);
              }
           }
           break;

        case GameEventType.round3FinalResult:
           print("[GameContainer] Final bonus result: ${event.data}");
           final data = event.data as Map<String, dynamic>?;
           if (data != null) {
              setState(() {
                if (data['total_score'] != null) {
                  _currentScore = data['total_score'];
                }
                _round3Score = data['bonus'] ?? _round3Score;
                _showDecision = false;
              });
              
              ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(content: Text("Round 3 Finished! Final Bonus: ${data['bonus']}")),
              );
           }
           break;

        case GameEventType.round3AllFinished:
           print("[GameContainer] Round 3 all finished: ${event.data}");
           final r3Data = event.data as Map<String, dynamic>?;
           if (r3Data?['players'] != null) {
              _updateLeaderboard(r3Data!['players']);
           } else if (r3Data?['rankings'] != null) {
              _updateLeaderboard(r3Data!['rankings']);
           }
           break;

        // Duplicate cases removed

        // Duplicate case removed

        case GameEventType.round2Product:
          print("[GameContainer] Product received: ${event.data}");
          _applyNextQuestion(event.data as Map<String, dynamic>?);
          break;

        case GameEventType.round2TurnResult:
           print("[GameContainer] Turn result: ${event.data}");
           final result = event.data as Map<String, dynamic>?;
           final int actualPrice = result?['correct_price'] ?? 0;
           final List bids = result?['bids'] ?? [];
           
           int maxScore = 0;
           String winnerName = "None";
           int myScoreDelta = 0;
           
           List<Map<String, dynamic>> updatedPlayers = List.from(_players);
           for (var b in bids) {
              int scoreDelta = b['score_delta'] ?? 0;
              int totalScore = b['total_score'] ?? 0;
              int id = b['account_id'] ?? 0;
              
              if (scoreDelta > maxScore) {
                maxScore = scoreDelta;
                winnerName = b['name'] ?? "Player";
              }
              
              for (int i = 0; i < updatedPlayers.length; i++) {
                if (updatedPlayers[i]['account_id'] == id) {
                   updatedPlayers[i]['score'] = totalScore;
                   break;
                }
              }
              
              if (_myPlayerId != null && id == _myPlayerId) {
                 myScoreDelta = scoreDelta;
                 _currentScore = totalScore;
              }
           }
           
           setState(() {
             updatedPlayers.sort((a, b) => (b['score'] ?? 0).compareTo(a['score'] ?? 0));
             _players = updatedPlayers;
           });

           ScaffoldMessenger.of(context).showSnackBar(
             SnackBar(
               content: Text("Price: $actualPrice | Winner: $winnerName (+$maxScore). Your total: $_currentScore"),
               duration: const Duration(seconds: 3),
               backgroundColor: myScoreDelta > 0 ? Colors.green : (maxScore > 0 ? Colors.orange : Colors.grey),
             ),
           );
           _showingResult = true;
           break;

        case GameEventType.round2AllFinished:
          print("[GameContainer] Round 2 finished event received: ${event.data}");
          final data = event.data as Map<String, dynamic>?;
          final int nextRound = data?['next_round'] ?? 3;
          
          if (data?['players'] != null) {
            setState(() {
              _players = List<Map<String, dynamic>>.from(data!['players']);
              // Sync our score
              if (_myPlayerId != null) {
                final me = _players.firstWhere((p) => p['id'] == _myPlayerId || p['account_id'] == _myPlayerId, orElse: () => {});
                if (me.isNotEmpty) {
                  _currentScore = me['score'] ?? me['points'] ?? _currentScore;
                }
              }
            });
          }

          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text("Round 2 Finished! Your Total Score: $_currentScore"), backgroundColor: Colors.purpleAccent),
          );

          if (nextRound > 0) {
            Future.delayed(const Duration(seconds: 3), () {
              if (mounted) {
                print("[GameContainer] Switching to round $nextRound");
                setState(() {
                  _currentRound = nextRound;
                  _currentQuestion = null;
                  _isLoading = true;
                });
                
                if (nextRound == 3) {
                  ServiceLocator.gameStateService.sendRound3PlayerReady(_matchId, 0);
                }
              }
            });
          }
          break;

        default:
          break;
      }
    });
  }



  void _applyNextQuestion(Map<String, dynamic>? data) {
    if (!mounted) return;
    
    setState(() {
      _currentQuestion = data;
      _isLoading = false;
      _showingResult = false; // Reset flag
    });
    
    _startCountdown();
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

  Widget _buildTimerSticker(int seconds) {
    return Container(
      width: 70,
      height: 70,
      decoration: BoxDecoration(
        color: const Color(0xFFE74C3C), // Red
        shape: BoxShape.circle,
        border: Border.all(color: Colors.white, width: 4),
        boxShadow: [
          BoxShadow(color: Colors.black.withOpacity(0.3), offset: const Offset(4, 4)),
        ],
      ),
      alignment: Alignment.center,
      child: Text(
        "$seconds",
        style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 32),
      ),
    );
  }

  Widget _buildScoreSticker(int score) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.end,
      children: [
        Container(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
          decoration: BoxDecoration(
            color: const Color(0xFF2ECC71), // Green
            borderRadius: BorderRadius.circular(12),
            border: Border.all(color: Colors.white, width: 4),
            boxShadow: [
              BoxShadow(color: Colors.black.withOpacity(0.3), offset: const Offset(4, 4)),
            ],
          ),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(
                "YOUR SCORE",
                style: GoogleFonts.luckiestGuy(color: Colors.white.withOpacity(0.8), fontSize: 12),
              ),
              Text(
                "$score",
                style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 28),
              ),
            ],
          ),
        ),
      ],
    );
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
            // LEFT SIDE: RANKING BOX (Sidebar) - Swapped to left
            Expanded(
              flex: 1,
              child: Padding(
                padding: const EdgeInsets.fromLTRB(30, 30, 0, 30),
                child: Column(
                  children: [
                    const SizedBox(height: 88), // Match top offset
                    Expanded(
                      child: Container(
                        decoration: BoxDecoration(
                          color: const Color(0xB32D3436), // Darker gray
                          borderRadius: BorderRadius.circular(24),
                          border: Border.all(color: Colors.black26, width: 4),
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

            // RIGHT SIDE: MAIN GAME AREA
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
                            _buildTimerSticker(_timeRemaining),
                            const SizedBox(width: 20),
                            _buildScoreSticker(_currentScore),
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
                          color: const Color(0xB3000000), // More transparent dark
                          borderRadius: BorderRadius.circular(24),
                          border: Border.all(color: Colors.white24, width: 2),
                        ),
                        child: _buildRoundWidget(),
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
    if (_inBonusRound) {
       return RoundBonusWidget(
         matchId: _matchId,
         myPlayerId: _myPlayerId ?? 0,
         onTransition: _handleBonusTransition,
       );
    }

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
        return Round3Widget(
          segments: _wheelSegments,
          score: _round3Score,
          spinNumber: _spinNumber,
          showDecision: _showDecision,
          onSpin: _handleSpin,
          onDecision: _handleDecision,
        );
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
  
  void _handleSpin() {
    print("[GameContainer] Spinning wheel");
    ServiceLocator.gameStateService.requestRound3Spin(_matchId);
  }
  
  void _handleDecision(bool continueSpin) {
    print("[GameContainer] Decision: continue=$continueSpin");
    ServiceLocator.gameStateService.submitRound3Decision(_matchId, continueSpin);
    
    // We don't manually update _currentScore here anymore.
    // Instead, we wait for GameEventType.round3FinalResult from backend
    // which is triggered by this decision.
    
    setState(() {
      _showDecision = false;
      if (continueSpin) {
        _round3Score = 0; // Clear score to indicate we are spinning again
      }
    });
  }

  void _handleBonusTransition(Map<String, dynamic> data) {
    print("[GameContainer] Bonus transition: $data");
    
    // Delay to let user see result
    Future.delayed(const Duration(seconds: 3), () {
      if (!mounted) return;
      
      setState(() {
        _inBonusRound = false;
        final nextPhase = data['next_phase'];
        
        if (nextPhase == 'NEXT_ROUND') {
          _currentRound = data['next_round'] ?? (_currentRound + 1);
          _isLoading = true;
          _currentQuestion = null;
          
          // Re-initialize for next round
          if (_currentRound == 2) {
             ServiceLocator.gameStateService.sendRound2PlayerReady(_matchId, 0);
          } else if (_currentRound == 3) {
             ServiceLocator.gameStateService.sendRound3PlayerReady(_matchId, 0);
          }
        } else if (nextPhase == 'MATCH_ENDED') {
          // Will be handled by ntfGameEnd
          print("[GameContainer] Bonus led to Match End");
        }
      });
    });
  }

  void _updateLeaderboard(List<dynamic>? playersData) {
    if (playersData == null) return;
    
    // Normalize and process player list
    List<Map<String, dynamic>> newPlayers = [];
    for (var p in playersData) {
      if (p is Map<String, dynamic>) {
        final newP = Map<String, dynamic>.from(p);
        
        // Handle ID variations
        final accId = newP['id'] ?? newP['account_id'];
        newP['id'] = accId;
        newP['account_id'] = accId;
        
        // Mark if this is me
        if (_myPlayerId != null && (accId == _myPlayerId || accId == _myPlayerId.toString())) {
          newP['isMe'] = true;
        } else {
          newP['isMe'] = false;
        }
        
        // Ensure name fallback if missing
        if (newP['name'] == null || newP['name'].toString().isEmpty) {
          newP['name'] = 'Player $accId';
        }

        // Map eliminated to isOut for sidebar
        newP['isOut'] = newP['eliminated'] == true || newP['is_out'] == true || newP['forfeited'] == true;
        
        newPlayers.add(newP);
      }
    }
    
    // Sort by score descending
    newPlayers.sort((a, b) => (b['score'] ?? 0).compareTo(a['score'] ?? 0));
    
    _players = newPlayers;
    
    // Sync my score
    if (_myPlayerId != null) {
       // We can find 'me' efficiently
       try {
         final me = newPlayers.firstWhere((p) => p['isMe'] == true);
         _currentScore = me['score'] ?? _currentScore;
       } catch (e) {
         // Not in list
       }
    }
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
                child: Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
                  child: Container(
                    padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
                    decoration: BoxDecoration(
                      color: isMe ? Colors.white.withOpacity(0.1) : Colors.black12,
                      borderRadius: BorderRadius.circular(12),
                      border: isMe ? Border.all(color: const Color(0xFFFFDE00), width: 2) : null,
                    ),
                    child: Row(
                      children: [
                        Expanded(
                          child: Text(
                            (p['name'] ?? 'Unknown').toUpperCase(),
                            style: GoogleFonts.luckiestGuy(
                              fontSize: 16,
                              color: Colors.white,
                              letterSpacing: 1,
                            ),
                            overflow: TextOverflow.ellipsis,
                          ),
                        ),
                        Text(
                          '${p['score'] ?? 0}',
                          style: GoogleFonts.luckiestGuy(
                            fontSize: 18,
                            color: const Color(0xFFFFDE00),
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
