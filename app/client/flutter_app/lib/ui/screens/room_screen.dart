import 'dart:async';
import 'package:flutter/material.dart';
import '../../services/service_locator.dart';
import '../../services/room_service.dart';
import '../../models/room.dart';
import '../theme/lobby_theme.dart';
import '../theme/room_theme.dart';
import '../widgets/user_card.dart';

class RoomScreen extends StatefulWidget {
  final int roomId;
  final bool initialIsHost;

  const RoomScreen({super.key, required this.roomId, this.initialIsHost = false});

  @override
  State<RoomScreen> createState() => _RoomScreenState();
}

class _RoomScreenState extends State<RoomScreen> {
  Room? _room;
  bool _isLoading = true;
  String? _error;
  int? _currentUserId;
  StreamSubscription? _eventSub;

  bool get _isHost => _room != null && _currentUserId != null && _room!.hostId == _currentUserId;

  @override
  void initState() {
    super.initState();
    _init();
    
  }

  @override
  void dispose() {
    _eventSub?.cancel();
    super.dispose();
  }

  Future<void> _init() async {
    final authState = await ServiceLocator.authService.getAuthState();
    if (authState["accountId"] != null) {
      _currentUserId = int.tryParse(authState["accountId"]!);
    }

    _eventSub = ServiceLocator.roomService.events.listen(_handleEvent);

    await _fetchRoom();
  }

  Future<void> _fetchRoom() async {
    if (!mounted) return;
    
    // --- MOCK SIMULATION FOR ROOM 999 ---
    // --- MOCK SIMULATION (ALWAYS ON FOR TESTING) ---
    // if (widget.roomId == 999) { // Disabled check to force mock for all 
      await Future.delayed(const Duration(seconds: 1)); // Fake network delay
      if (mounted) {
        setState(() {
          _room = Room(
            id: widget.roomId, // Use the ID passed in, but mock data
            name: "Test Room ${widget.roomId}", 
            code: "TEST${widget.roomId}",
            hostId: 10,
            maxPlayers: 5,
            visibility: "public",
            mode: "scoring",
            wagerMode: true,
            roundTime: 15,
            members: [
              RoomMember(accountId: 10, email: "HostUser", ready: true, avatar: null),
              RoomMember(accountId: 123, email: "PlayerTwo", ready: false, avatar: null),
              RoomMember(accountId: 14, email: "Alice", ready: true, avatar: null),
              RoomMember(accountId: 15, email: "Bob", ready: false, avatar: null),
            ]
          );
          _isLoading = false;
          _currentUserId = 123; // Simulate being PlayerTwo (non-host)
        });
      }
      return;
    // }
    // ------------------------------------

    try {
      final room = await ServiceLocator.roomService.fetchRoom(widget.roomId);
      if (mounted) {
        setState(() {
          _room = room;
          _isLoading = false;
          if (room == null) {
            _error = "Room not found or error loading room";
          }
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _isLoading = false;
          _error = "Error: $e";
        });
      }
    }
  }

  void _handleEvent(RoomEvent event) {
    if (!mounted) return;
    
    switch (event.type) {
      case RoomEventType.playerJoined:
      case RoomEventType.playerLeft:
      case RoomEventType.memberKicked:
      case RoomEventType.rulesChanged:
        _fetchRoom(); // Refresh data
        break;
      case RoomEventType.roomClosed:
        _showExitDialog("Room Closed", "The host has closed the room.");
        break;
      case RoomEventType.gameStarted:
        // TODO: Navigate to Game Screen
         ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text("Game Starting!")),
        );
        break;
    }
  }

  void _showExitDialog(String title, String content) {
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (_) => AlertDialog(
        title: Text(title),
        content: Text(content),
        actions: [
          TextButton(
            onPressed: () {
              Navigator.pop(context); // Close dialog
              Navigator.pop(context); // Leave screen
            },
            child: const Text("OK"),
          )
        ],
      ),
    );
  }

  Future<void> _leaveRoom() async {
    await ServiceLocator.roomService.leaveRoom(widget.roomId);
    if (mounted) Navigator.pop(context);
  }

  Future<void> _kickMember(int targetId) async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (_) => AlertDialog(
        title: const Text("Kick Member"),
        content: const Text("Are you sure you want to kick this player?"),
        actions: [
           TextButton(onPressed: () => Navigator.pop(context, false), child: const Text("Cancel")),
           TextButton(onPressed: () => Navigator.pop(context, true), child: const Text("Kick", style: TextStyle(color: Colors.red))),
        ],
      ),
    );

    if (confirm == true) {
      await ServiceLocator.roomService.kickMember(widget.roomId, targetId);
    }
  }
  
  Future<void> _startGame() async {
      await ServiceLocator.roomService.startGame(widget.roomId);
  }

  @override
  Widget build(BuildContext context) {
    if (_isLoading) {
       return const Scaffold(body: Center(child: CircularProgressIndicator()));
    }

    if (_error != null || _room == null) {
      return Scaffold(
        appBar: AppBar(title: const Text("Error")),
        body: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(_error ?? "Unknown error"),
              const SizedBox(height: 20),
              ElevatedButton(onPressed: () => Navigator.pop(context), child: const Text("Go Back")),
            ],
          ),
        ),
      );
    }

    return Scaffold(
      body: Stack(
        children: [
          // Background
          Container(
            width: double.infinity,
            height: double.infinity,
            decoration: const BoxDecoration(
              image: DecorationImage(
                image: AssetImage('assets/images/waitingroom.png'),
                fit: BoxFit.cover,
              ),
            ),
          ),

          const Positioned(
            top: 50,
            left: 30,
            child: UserCard(),
          ),

          SafeArea(
            child: Column(
              children: [
                _buildHeader(),
                Expanded(
                  child: Padding(
                    padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 20),
                    child: LayoutBuilder( // Responsive check
                      builder: (context, constraints) {
                        // If width is wide, uses row. If narrow, use column.
                        final isWide = constraints.maxWidth > 800;
                        if (isWide) {
                          return Row(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Expanded(flex: 1, child: _buildGameRulesPanel()), // Ratio 1
                              const SizedBox(width: 20),
                              Expanded(flex: 3, child: _buildMemberListPanel()), // Ratio 3
                              const SizedBox(width: 20),
                              SizedBox(width: 180, child: _buildActionButtons()), // Fixed width 180
                            ],
                          );
                        } else {
                          // Mobile/Narrow layout
                          return SingleChildScrollView(
                            child: Column(
                               children: [
                                  _buildMemberListPanel(),
                                  const SizedBox(height: 20),
                                  _buildGameRulesPanel(),
                                  const SizedBox(height: 20),
                                  _buildActionButtons(),
                               ],
                            ),
                          );
                        }
                      },
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHeader() {
    return Padding(
      padding: const EdgeInsets.only(top: 20),
      child: Column(
        children: [
          const Text(
            "THE PRICE IS RIGHT",
            style: TextStyle(
              fontFamily: 'LuckiestGuy',
              fontSize: 40,
              color: RoomTheme.primaryDark,
              shadows: [
                Shadow(offset: Offset(2, 2), color: Colors.white),
                Shadow(offset: Offset(4, 4), color: Color(0xFFFFAA00)),
              ],
            ),
          ),
          if (_room != null)
            Text(
              "Room ID: ${_room!.code ?? _room!.id}",
              style: const TextStyle(
                fontFamily: 'LuckiestGuy',
                fontSize: 20,
                color: Colors.white,
                 shadows: [Shadow(offset: Offset(1, 1), color: Colors.black)],
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildGameRulesPanel() {
    return LayoutBuilder(
      builder: (context, constraints) {
        final w = constraints.maxWidth;
        final titleSize = (w * 0.12).clamp(18.0, 32.0);
        final labelSize = (w * 0.08).clamp(12.0, 20.0);
        final valueSize = (w * 0.08).clamp(12.0, 20.0);

        return Container(
          decoration: RoomTheme.panelDecoration,
          padding: const EdgeInsets.all(20),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text("GAME RULES", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: titleSize, color: const Color(0xFFFFEB3B))),
              const SizedBox(height: 20),
              _ruleRow("MAX PLAYERS", "6", labelSize, valueSize),
              _ruleRow("VISIBILITY", "PUBLIC", labelSize, valueSize),
              _ruleRow("MODE", "SCORING", labelSize, valueSize),
              _ruleRow("ROUND TIME", "15S", labelSize, valueSize),
              _ruleRow("WAGER", "ON", labelSize, valueSize),
            ],
          ),
        );
      }
    );
  }

  Widget _buildMemberListPanel() {
    return Container(
      decoration: RoomTheme.panelDecoration,
      padding: const EdgeInsets.all(20),
      child: Column(
        mainAxisSize: MainAxisSize.min, // Giúp pannel ôm sát nội dung
        children: [
          Text(_room?.name ?? "ROOM", style: RoomTheme.titleStyle),
          const SizedBox(height: 15),
          // Không dùng Expanded để GridView tự định nghĩa chiều cao theo nội dung
          GridView.count(
            shrinkWrap: true,
            crossAxisCount: 3,
            crossAxisSpacing: 12,
            mainAxisSpacing: 12,
            childAspectRatio: 1.25, // Tăng tỉ lệ để Card rộng và ngắn lại, fit 6 ô đẹp hơn
            physics: const NeverScrollableScrollPhysics(),
            children: List.generate(6, (index) {
              if (_room != null && index < _room!.members.length) {
                return _buildPlayerCard(_room!.members[index]);
              } else {
                return _buildEmptySlot();
              }
            }),
          ),
          const SizedBox(height: 15),
          Text(
            "PLAYERS (${_room?.members.length ?? 0}/6)", 
            style: const TextStyle(fontFamily: 'LuckiestGuy', color: Colors.white, fontSize: 16)
          ),
        ],
      ),
    );
  }

  Widget _buildPlayerCard(RoomMember member) {
    final isHostMember = member.accountId == _room!.hostId;
    final statusColor = member.ready ? const Color(0xFF8BC34A) : const Color(0xFFFFB74D);

    return LayoutBuilder(
      builder: (context, constraints) {
        final w = constraints.maxWidth;
        // Calculate responsive sizes
        final double avatarRadius = w * 0.22; 
        final double nameFontSize = (w * 0.10).clamp(10.0, 24.0);
        final double statusFontSize = (w * 0.07).clamp(8.0, 16.0);
        final double crownSize = w * 0.25;

        return Container(
          decoration: BoxDecoration(
            color: Colors.white,
            borderRadius: BorderRadius.circular(15),
            border: Border.all(
              color: member.ready ? statusColor : Colors.white24, 
              width: 3
            ),
          ),
          child: Stack(
            children: [
              Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  CircleAvatar(
                    radius: avatarRadius,
                    backgroundColor: const Color(0xFFE0E0E0),
                    backgroundImage: member.avatar != null 
                      ? NetworkImage(member.avatar!) 
                      : const AssetImage('assets/images/default-mushroom.jpg') as ImageProvider,
                    child: member.avatar == null 
                       ? const SizedBox.shrink()
                       : null,
                  ),
                  SizedBox(height: w * 0.04),
                  Padding(
                    padding: const EdgeInsets.symmetric(horizontal: 4),
                    child: Text(
                      member.email.toUpperCase(),
                      style: TextStyle(
                        fontFamily: 'LuckiestGuy',
                        fontSize: nameFontSize,
                        color: const Color(0xFF1F2A44),
                      ),
                      textAlign: TextAlign.center,
                      overflow: TextOverflow.ellipsis,
                    ),
                  ),
                  SizedBox(height: w * 0.03),
                  Container(
                    width: double.infinity,
                    margin: const EdgeInsets.symmetric(horizontal: 12),
                    padding: const EdgeInsets.symmetric(vertical: 4),
                    decoration: BoxDecoration(
                      color: statusColor,
                      borderRadius: BorderRadius.circular(6),
                    ),
                    child: Text(
                      member.ready ? "READY" : "WAITING",
                      textAlign: TextAlign.center,
                      style: TextStyle(
                        fontFamily: 'LuckiestGuy',
                        fontSize: statusFontSize,
                        color: Colors.white,
                      ),
                    ),
                  ),
                ],
              ),
              if (isHostMember)
                Positioned(
                  top: -crownSize * 0.15,
                  left: -crownSize * 0.10,
                  child: Transform.rotate(
                    angle: -0.2,
                    child: Image.asset(
                      'assets/images/crown.png',
                      width: crownSize,
                      height: crownSize,
                      errorBuilder: (context, error, stackTrace) => Icon(Icons.star, color: Colors.amber, size: crownSize * 0.8),
                    ),
                  ),
                ),
            ],
          ),
        );
      },
    );
  }

  Widget _buildEmptySlot() {
    return Container(
      decoration: BoxDecoration(
        color: Colors.black.withOpacity(0.3),
        borderRadius: BorderRadius.circular(15),
        border: Border.all(color: Colors.white10, width: 2),
      ),
      child: const Center(
        child: Text(
          "EMPTY",
          style: TextStyle(
            fontFamily: 'LuckiestGuy',
            color: Colors.white24,
            fontSize: 16, // Chữ Empty to hơn cho cân đối
          ),
        ),
      ),
    );
  }

  Widget _buildActionButtons() {
    return Column(
      children: [
        if (_isHost)
          _actionButton("START GAME", RoomTheme.accentYellow, RoomTheme.primaryDark, _startGame),
          const SizedBox(height: 10),
        
        _actionButton("INVITE FRIENDS", RoomTheme.accentGreen, Colors.white, () {}),
        const SizedBox(height: 10),
        _actionButton("LEAVE ROOM", RoomTheme.accentRed, Colors.white, _leaveRoom),
      ],
    );
  }

  Widget _actionButton(String label, Color bg, Color textCol, VoidCallback onPressed) {
    return SizedBox(
      width: double.infinity,
      child: ElevatedButton(
        style: ElevatedButton.styleFrom(
          backgroundColor: bg,
          side: const BorderSide(color: RoomTheme.primaryDark, width: 2),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
          padding: const EdgeInsets.symmetric(vertical: 15),
        ),
        onPressed: onPressed,
        child: Text(label, style: TextStyle(fontFamily: 'LuckiestGuy', color: textCol)),
      ),
    );
  }

  Widget _ruleRow(String label, String value, double labelSize, double valueSize) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: TextStyle(fontFamily: 'LuckiestGuy', color: const Color(0xFF29B6F6), fontSize: labelSize)),
          Text(value, style: TextStyle(fontFamily: 'LuckiestGuy', color: RoomTheme.accentYellow, fontSize: valueSize)),
        ],
      ),
    );
  }
}
