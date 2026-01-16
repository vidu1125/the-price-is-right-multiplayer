import 'dart:async';
import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import '../../services/service_locator.dart';
import '../../services/bonus_service.dart';

class RoundBonusWidget extends StatefulWidget {
  final int matchId;
  final int myPlayerId;
  final Function(Map<String, dynamic>)? onTransition;

  const RoundBonusWidget({
    super.key,
    required this.matchId,
    required this.myPlayerId,
    this.onTransition,
  });

  @override
  State<RoundBonusWidget> createState() => _RoundBonusWidgetState();
}

enum BonusState { drawing, revealing, completed }
enum PlayerBonusState { waitingToDraw, cardDrawn }

class _RoundBonusWidgetState extends State<RoundBonusWidget> {
  String _role = 'spectator';
  String _bonusType = 'elimination';
  BonusState _bonusState = BonusState.drawing;
  PlayerBonusState _playerState = PlayerBonusState.waitingToDraw;

  List<dynamic> _participants = [];
  List<Map<String, dynamic>> _drawnPlayers = [];
  int _cardsRemaining = 0;
  int _totalCards = 0;

  List<dynamic> _revealedCards = [];
  Map<String, dynamic>? _eliminatedPlayer;
  Map<String, dynamic>? _winner;

  String _message = 'Watching bonus round...';
  bool _isDrawing = false;
  bool _showResult = false;
  int? _dealingToPlayer;

  StreamSubscription? _eventSub;

  @override
  void initState() {
    super.initState();
    _eventSub = ServiceLocator.bonusService.events.listen(_handleBonusEvent);
    ServiceLocator.bonusService.sendBonusReady(widget.matchId);
  }

  @override
  void dispose() {
    _eventSub?.cancel();
    super.dispose();
  }

  void _handleBonusEvent(BonusEvent event) {
    if (!mounted) return;

    setState(() {
      switch (event.type) {
        case BonusEventType.init:
          final data = event.data;
          _role = data['role'] ?? 'participant';
          _bonusType = data['bonus_type'] ?? 'elimination';
          _bonusState = _mapState(data['state']);
          _cardsRemaining = data['cards_remaining'] ?? data['participant_count'] ?? 2;
          _totalCards = data['participant_count'] ?? 2;
          if (_role == 'participant') {
            _playerState = _mapPlayerState(data['player_state']);
          }
          break;

        case BonusEventType.participant:
          final data = event.data;
          _role = 'participant';
          _bonusType = data['bonus_type'] ?? 'elimination';
          _participants = data['participants'] ?? [];
          _cardsRemaining = data['total_cards'] ?? _participants.length;
          _totalCards = data['total_cards'] ?? _participants.length;
          _message = data['instruction'] ?? 'Click DRAW to draw a card';
          _bonusState = BonusState.drawing;
          _playerState = PlayerBonusState.waitingToDraw;
          break;

        case BonusEventType.spectator:
          final data = event.data;
          _role = 'spectator';
          _bonusType = data['bonus_type'] ?? 'elimination';
          _participants = data['participants'] ?? [];
          _totalCards = _participants.length;
          _cardsRemaining = _participants.length;
          _message = data['message'] ?? 'Watching bonus round...';
          _bonusState = BonusState.drawing;
          break;

        case BonusEventType.cardDrawn:
          _isDrawing = false;
          _playerState = PlayerBonusState.cardDrawn;
          _message = 'Card drawn! Waiting for others...';
          break;

        case BonusEventType.playerDrew:
          final data = event.data;
          _dealingToPlayer = data['player_id'];
          Future.delayed(const Duration(milliseconds: 600), () {
            if (mounted) {
              setState(() {
                _dealingToPlayer = null;
                _drawnPlayers.add({
                  'id': data['player_id'],
                  'name': data['player_name'],
                  'drawnAt': data['drawn_at']
                });
                _cardsRemaining = (_cardsRemaining - 1).clamp(0, 100);
                
                for (var p in _participants) {
                  if (p['account_id'] == data['player_id']) {
                    p['hasDrawn'] = true;
                  }
                }
              });
            }
          });
          break;

        case BonusEventType.reveal:
          final data = event.data;
          _bonusState = BonusState.revealing;
          _revealedCards = data['results'] ?? [];
          _eliminatedPlayer = data['eliminated_player'];
          _winner = data['winner'];
          
          Future.delayed(const Duration(milliseconds: 500), () {
            if (mounted) setState(() => _showResult = true);
          });
          break;

        case BonusEventType.transition:
          _bonusState = BonusState.completed;
          if (widget.onTransition != null) {
            widget.onTransition!(event.data);
          }
          break;
          
        default:
          break;
      }
    });
  }

  BonusState _mapState(dynamic state) {
    if (state is int) {
      if (state == 3) return BonusState.revealing;
      if (state == 5) return BonusState.completed;
      return BonusState.drawing;
    }
    // Fallback for String (if changed later)
    if (state == 'REVEALING') return BonusState.revealing;
    if (state == 'COMPLETED') return BonusState.completed;
    return BonusState.drawing;
  }

  PlayerBonusState _mapPlayerState(dynamic state) {
    if (state is int) {
      if (state == 1) return PlayerBonusState.cardDrawn;
      return PlayerBonusState.waitingToDraw;
    }
    if (state == 'CARD_DRAWN') return PlayerBonusState.cardDrawn;
    return PlayerBonusState.waitingToDraw;
  }

  void _drawCard() {
    if (_isDrawing || _playerState != PlayerBonusState.waitingToDraw) return;
    setState(() => _isDrawing = true);
    ServiceLocator.bonusService.drawCard(widget.matchId);
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.black.withOpacity(0.7),
        borderRadius: BorderRadius.circular(24),
      ),
      child: Column(
        children: [
          _buildHeader(),
          Expanded(
            child: Stack(
              children: [
                _buildGameArea(),
                if (_showResult) _buildResultOverlay(),
              ],
            ),
          ),
          _buildFooter(),
        ],
      ),
    );
  }

  Widget _buildHeader() {
    final typeLabel = _bonusType == 'elimination' 
        ? 'ELIMINATION TIEBREAKER' 
        : 'WINNER TIEBREAKER';
    final subtitle = _bonusType == 'elimination'
        ? 'Multiple players tied at lowest score. One will be eliminated!'
        : 'Multiple players tied at highest score. The winner will be decided!';

    return Padding(
      padding: const EdgeInsets.all(16.0),
      child: Column(
        children: [
          Text(
            typeLabel,
            style: GoogleFonts.luckiestGuy(fontSize: 32, color: const Color(0xFFFFDE00)),
          ),
          Text(
            subtitle,
            textAlign: TextAlign.center,
            style: const TextStyle(color: Colors.white70, fontSize: 16),
          ),
        ],
      ),
    );
  }

  Widget _buildGameArea() {
    final player1 = _participants.isNotEmpty ? _participants[0] : null;
    final player2 = _participants.length > 1 ? _participants[1] : null;

    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
      children: [
        Expanded(child: _buildPlayerBox(player1, 'left')),
        _buildCenterDeck(),
        Expanded(child: _buildPlayerBox(player2, 'right')),
      ],
    );
  }

  Widget _buildPlayerBox(dynamic player, String position) {
    if (player == null) {
      return Container(
        margin: const EdgeInsets.all(8),
        decoration: BoxDecoration(
          color: Colors.white.withOpacity(0.05),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: Colors.white12, width: 2),
        ),
        child: const Center(child: Text('Waiting...', style: TextStyle(color: Colors.white24))),
      );
    }

    final int accountId = player['account_id'];
    final bool hasDrawn = _drawnPlayers.any((d) => d['id'] == accountId) || player['hasDrawn'] == true;
    final bool isMe = accountId == widget.myPlayerId;
    final bool isDealing = _dealingToPlayer == accountId;
    final dynamic revealedCard = _revealedCards.firstWhere((r) => r['player_id'] == accountId, orElse: () => null);
    final bool isEliminated = revealedCard != null && revealedCard['card'] == 'eliminated';

    return Container(
      margin: const EdgeInsets.all(12),
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: isMe ? const Color(0xFF2980B9).withOpacity(0.2) : Colors.white.withOpacity(0.05),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
          color: isMe ? const Color(0xFF3498DB) : Colors.white24,
          width: 2,
        ),
        boxShadow: isMe ? [BoxShadow(color: Colors.blue.withOpacity(0.3), blurRadius: 10)] : [],
      ),
      child: Column(
        children: [
          Row(
            children: [
              CircleAvatar(
                backgroundColor: isMe ? Colors.blue : Colors.grey,
                child: Text(player['name']?[0] ?? '?', style: const TextStyle(color: Colors.white)),
              ),
              const SizedBox(width: 8),
              Expanded(
                child: Text(
                  "${player['name']}${isMe ? ' (YOU)' : ''}",
                  style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 16),
                  overflow: TextOverflow.ellipsis,
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          Expanded(
            child: Center(
              child: _buildCard(hasDrawn, isDealing, revealedCard, isEliminated),
            ),
          ),
          const SizedBox(height: 12),
          _buildStatusTag(hasDrawn, revealedCard, isEliminated),
        ],
      ),
    );
  }

  Widget _buildCard(bool hasDrawn, bool isDealing, dynamic revealedCard, bool isEliminated) {
    // Dealing animation - card flying in
    if (isDealing) {
      return TweenAnimationBuilder<double>(
        tween: Tween(begin: 0.0, end: 1.0),
        duration: const Duration(milliseconds: 600),
        builder: (context, value, child) {
          return Transform.translate(
            offset: Offset(0, 100 * (1 - value)),
            child: Opacity(
              opacity: value,
              child: Container(
                decoration: BoxDecoration(
                  color: const Color(0xFF34495E),
                  borderRadius: BorderRadius.circular(16),
                  border: Border.all(color: Colors.white24, width: 3),
                  boxShadow: [
                    BoxShadow(color: Colors.black.withOpacity(0.3), blurRadius: 8),
                  ],
                ),
                child: Center(
                  child: Text(
                    '?',
                    style: GoogleFonts.luckiestGuy(fontSize: 64, color: Colors.white24),
                  ),
                ),
              ),
            ),
          );
        },
      );
    }

    // Reveal animation - flip card to show result
    if (_showResult && revealedCard != null) {
      return TweenAnimationBuilder<double>(
        tween: Tween(begin: 0.0, end: 1.0),
        duration: const Duration(milliseconds: 800),
        builder: (context, value, child) {
          final bool showFront = value > 0.5;
          return Transform(
            transform: Matrix4.identity()..rotateY(value * 3.14159),
            alignment: Alignment.center,
            child: showFront
              ? Transform(
                  transform: Matrix4.identity()..rotateY(3.14159),
                  alignment: Alignment.center,
                  child: Container(
                    decoration: BoxDecoration(
                      gradient: LinearGradient(
                        begin: Alignment.topLeft,
                        end: Alignment.bottomRight,
                        colors: isEliminated 
                          ? [const Color(0xFFC0392B), const Color(0xFF8E44AD)]
                          : [const Color(0xFF27AE60), const Color(0xFF16A085)],
                      ),
                      borderRadius: BorderRadius.circular(16),
                      border: Border.all(color: Colors.white, width: 4),
                      boxShadow: [
                        BoxShadow(
                          color: (isEliminated ? Colors.red : Colors.green).withOpacity(0.5),
                          blurRadius: 20,
                          spreadRadius: 2,
                        ),
                      ],
                    ),
                    child: Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Icon(
                            isEliminated ? Icons.close : Icons.check,
                            size: 48,
                            color: Colors.white,
                          ),
                          const SizedBox(height: 8),
                          Text(
                            isEliminated ? 'ELIMINATED' : 'SAFE',
                            style: GoogleFonts.luckiestGuy(
                              fontSize: 28,
                              color: Colors.white,
                              shadows: [
                                const Shadow(
                                  color: Colors.black45,
                                  offset: Offset(2, 2),
                                  blurRadius: 4,
                                ),
                              ],
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                )
              : Container(
                  decoration: BoxDecoration(
                    color: const Color(0xFF34495E),
                    borderRadius: BorderRadius.circular(16),
                    border: Border.all(color: Colors.white24, width: 3),
                  ),
                  child: Center(
                    child: Text(
                      '?',
                      style: GoogleFonts.luckiestGuy(fontSize: 64, color: Colors.white24),
                    ),
                  ),
                ),
          );
        },
      );
    }

    // Card drawn but not revealed yet
    if (hasDrawn) {
      return Container(
        decoration: BoxDecoration(
          color: const Color(0xFF34495E),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: Colors.white24, width: 3),
          boxShadow: [
            BoxShadow(color: Colors.black.withOpacity(0.2), blurRadius: 8),
          ],
        ),
        child: Center(
          child: Text(
            '?',
            style: GoogleFonts.luckiestGuy(fontSize: 64, color: Colors.white24),
          ),
        ),
      );
    }

    // Empty slot - waiting to draw
    return Container(
      decoration: BoxDecoration(
        color: Colors.black26,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: Colors.white10, width: 2, style: BorderStyle.solid),
      ),
      child: const Center(
        child: Icon(Icons.help_outline, color: Colors.white10, size: 64),
      ),
    );
  }

  Widget _buildStatusTag(bool hasDrawn, dynamic revealedCard, bool isEliminated) {
    String text = 'Waiting...';
    Color color = Colors.grey;
    if (_showResult && revealedCard != null) {
      text = isEliminated ? '💀 ELIMINATED' : '✓ SAFE';
      color = isEliminated ? Colors.red : Colors.green;
    } else if (hasDrawn) {
      text = '✓ Card Drawn';
      color = Colors.blue;
    }

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
      decoration: BoxDecoration(
        color: color.withOpacity(0.2),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: color.withOpacity(0.5)),
      ),
      child: Text(text, style: GoogleFonts.luckiestGuy(color: color, fontSize: 14)),
    );
  }

  Widget _buildCenterDeck() {
    final bool canDraw = _role == 'participant' && _playerState == PlayerBonusState.waitingToDraw && !_isDrawing;

    return Container(
      width: 200,
      padding: const EdgeInsets.all(16),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          // Card stack representation
          Stack(
            alignment: Alignment.center,
            children: List.generate(
              _cardsRemaining.clamp(0, 5),
              (i) => Transform.translate(
                offset: Offset(i * 3.0, -i * 5.0),
                child: Transform.rotate(
                  angle: i * 0.02,
                  child: Container(
                    width: 140,
                    height: 180,
                    decoration: BoxDecoration(
                      color: const Color(0xFF34495E),
                      borderRadius: BorderRadius.circular(12),
                      border: Border.all(color: Colors.white24, width: 2),
                      boxShadow: [
                        BoxShadow(
                          color: Colors.black.withOpacity(0.3),
                          blurRadius: 4,
                          offset: const Offset(0, 2),
                        ),
                      ],
                    ),
                    child: Center(
                      child: Text(
                        '?',
                        style: GoogleFonts.luckiestGuy(
                          fontSize: 48,
                          color: Colors.white24,
                        ),
                      ),
                    ),
                  ),
                ),
              ),
            ),
          ),
          const SizedBox(height: 24),
          if (canDraw && !_showResult)
            _buildDrawButton()
          else if (_role == 'participant' && _playerState == PlayerBonusState.cardDrawn && !_showResult)
            const Text('⏳ Waiting for others...', style: TextStyle(color: Colors.white70))
          else if (!_showResult)
            Text('$_cardsRemaining / $_totalCards left', style: const TextStyle(color: Colors.white38)),
        ],
      ),
    );
  }

  Widget _buildDrawButton() {
    return InkWell(
      onTap: _drawCard,
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
        decoration: BoxDecoration(
          gradient: const LinearGradient(colors: [Color(0xFFF1C40F), Color(0xFFF39C12)]),
          borderRadius: BorderRadius.circular(30),
          boxShadow: [BoxShadow(color: Colors.orange.withOpacity(0.4), blurRadius: 10, offset: const Offset(0, 4))],
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            if (_isDrawing)
              const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(color: Colors.white, strokeWidth: 2))
            else
              const Icon(Icons.style, color: Colors.white),
            const SizedBox(width: 8),
            Text(
              _isDrawing ? 'DRAWING...' : 'DRAW CARD',
              style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 18),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildResultOverlay() {
    final String winnerName = _winner?['name'] ?? 'Unknown';
    final String eliminatedName = _eliminatedPlayer?['name'] ?? 'Unknown';

    return Positioned.fill(
      child: Container(
        color: Colors.black54,
        child: Center(
          child: TweenAnimationBuilder<double>(
            tween: Tween(begin: 0.0, end: 1.0),
            duration: const Duration(milliseconds: 500),
            builder: (context, value, child) {
              return Transform.scale(
                scale: value,
                child: Container(
                  padding: const EdgeInsets.all(40),
                  decoration: BoxDecoration(
                    color: _bonusType == 'elimination' ? Colors.red.shade900.withOpacity(0.9) : Colors.amber.shade900.withOpacity(0.9),
                    borderRadius: BorderRadius.circular(32),
                    border: Border.all(color: Colors.white24, width: 4),
                  ),
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Text(
                        _bonusType == 'elimination' ? 'ELIMINATED' : 'WINNER!',
                        style: GoogleFonts.luckiestGuy(fontSize: 48, color: Colors.white),
                      ),
                      const SizedBox(height: 8),
                      Text(
                        _bonusType == 'elimination' ? eliminatedName : winnerName,
                        style: GoogleFonts.luckiestGuy(fontSize: 32, color: const Color(0xFFFFDE00)),
                      ),
                    ],
                  ),
                ),
              );
            },
          ),
        ),
      ),
    );
  }

  Widget _buildFooter() {
    return Padding(
      padding: const EdgeInsets.all(16.0),
      child: Text(
        _showResult ? "Transitioning to next phase..." : _message,
        style: const TextStyle(color: Colors.white60),
      ),
    );
  }
}
