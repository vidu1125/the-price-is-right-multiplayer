import 'dart:async';
import 'package:flutter/material.dart';
import '../../services/service_locator.dart';
import '../../services/room_service.dart';
import '../../services/game_service.dart';
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
          _currentUserId = 10; // Simulate being Host (changed from 123)
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
    final result = await ServiceLocator.gameService.startGame(widget.roomId);
    if (result['success'] != true) {
        if (mounted) {
            ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(content: Text("Failed to start game: ${result['error']}")),
            );
        }
    }
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

          SafeArea(
            child: Column(
              children: [
                // Top bar with Header
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 10),
                  child: Row(
                    children: [
                      Expanded(child: _buildHeader()),
                    ],
                  ),
                ),
                Expanded(
                  child: LayoutBuilder(
                    builder: (context, constraints) {
                      final double screenWidth = MediaQuery.of(context).size.width;
                      final double screenHeight = MediaQuery.of(context).size.height;
                      final bool isWide = screenWidth > 900;

                      // Responsive padding and spacing
                      final double horizontalPadding = isWide ? screenWidth * 0.05 : 20.0;
                      final double verticalPadding = isWide ? screenHeight * 0.04 : 20.0;
                      final double columnSpacing = isWide ? screenWidth * 0.03 : 20.0;

                      return Padding(
                        padding: EdgeInsets.symmetric(horizontal: horizontalPadding, vertical: verticalPadding),
                        child: isWide
                            ? Row(
                                key: const ValueKey('wide_layout'),
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Expanded(flex: 2, child: _buildGameRulesPanel()),
                                  SizedBox(width: columnSpacing),
                                  Expanded(flex: 5, child: _buildMemberListPanel()),
                                  SizedBox(width: columnSpacing),
                                  SizedBox(width: 280, child: _buildActionButtons()),
                                ],
                              )
                            : SingleChildScrollView(
                                key: const ValueKey('narrow_layout'),
                                child: Column(
                                  children: [
                                    _buildMemberListPanel(),
                                    const SizedBox(height: 20),
                                    _buildGameRulesPanel(),
                                    const SizedBox(height: 20),
                                    _buildActionButtons(),
                                  ],
                                ),
                              ),
                      );
                    },
                  ),
                ),
              ],
            ),
          ),
          // 1. User Card (Góc trên bên trái - Responsive positioning)
          Positioned(
            top: MediaQuery.of(context).size.height * 0.02,
            left: MediaQuery.of(context).size.width * 0.02,
            child: const UserCard(),
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
          const SizedBox(height: 50),
          Text(
            "THE PRICE IS RIGHT",
            style: TextStyle(
              fontFamily: 'LuckiestGuy',
              fontSize: (MediaQuery.of(context).size.width * 0.06).clamp(48.0, 96.0), // Increased from 0.05 and 40-80
              color: RoomTheme.primaryDark,
              shadows: const [
                Shadow(offset: Offset(2, 2), color: Colors.white),
                Shadow(offset: Offset(4, 4), color: Color(0xFFFFAA00)),
              ],
            ),
          ),
          if (_room != null)
            Text(
              "Room ID: ${_room!.code ?? _room!.id}",
              style: TextStyle(
                fontFamily: 'LuckiestGuy',
                fontSize: (MediaQuery.of(context).size.width * 0.02).clamp(18.0, 28.0), // Responsive
                color: Colors.white,
                 shadows: const [Shadow(offset: Offset(1, 1), color: Colors.black)],
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
        final titleSize = (w * 0.12).clamp(20.0, 36.0);
        final labelSize = (w * 0.12).clamp(14.0, 24.0); // Increased from 0.08
        final valueSize = (w * 0.12).clamp(14.0, 24.0); // Increased from 0.08
        final containerPadding = (w * 0.08).clamp(12.0, 24.0); // Responsive padding
        final itemSpacing = (w * 0.05).clamp(8.0, 20.0); // Responsive spacing

        return Stack(
          children: [
            Container(
              decoration: RoomTheme.panelDecoration,
              padding: EdgeInsets.all(containerPadding),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(
                    "GAME RULES", 
                    style: TextStyle(
                      fontFamily: 'LuckiestGuy', 
                      fontSize: titleSize, 
                      color: const Color(0xFFFFEB3B)
                    )
                  ),
                  SizedBox(height: itemSpacing),
                  _ruleRow("MAX PLAYERS", "6", labelSize, valueSize),
                  _ruleRow("VISIBILITY", "PUBLIC", labelSize, valueSize),
                  _ruleRow("MODE", "SCORING", labelSize, valueSize),
                  _ruleRow("ROUND TIME", "15S", labelSize, valueSize),
                  _ruleRow("WAGER", "ON", labelSize, valueSize),
                ],
              ),
            ),
            // Nút EDIT floating ở góc phải trên (chỉ host thấy)
            if (_isHost)
              Positioned(
                top: 8,
                right: 8,
                child: InkWell(
                  onTap: () {
                    // TODO: Mở dialog edit game rules
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(content: Text("Edit Rules - Coming Soon")),
                    );
                  },
                  child: Container(
                    padding: const EdgeInsets.all(8),
                    decoration: BoxDecoration(
                      color: const Color(0xFF29B6F6),
                      shape: BoxShape.circle,
                      border: Border.all(color: Colors.white, width: 2),
                      boxShadow: [
                        BoxShadow(
                          color: Colors.black.withOpacity(0.3),
                          blurRadius: 6,
                          spreadRadius: 1,
                        ),
                      ],
                    ),
                    child: const Icon(
                      Icons.edit,
                      color: Colors.white,
                      size: 18,
                    ),
                  ),
                ),
              ),
          ],
        );
      }
    );
  }

  Widget _buildMemberListPanel() {
    return LayoutBuilder(
      builder: (context, constraints) {
        // Calculate size based on actual panel space
        double panelWidth = constraints.maxWidth;
        double cardSpacing = (panelWidth * 0.03).clamp(10.0, 20.0); // Giảm từ 0.04 và 12-25
        double containerPadding = (panelWidth * 0.025).clamp(12.0, 24.0); // Giảm từ 0.03 và 15-30

        return Container(
          decoration: RoomTheme.panelDecoration,
          padding: EdgeInsets.all(containerPadding),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(
                _room?.name ?? "ROOM", 
                style: RoomTheme.titleStyle.copyWith(
                  fontSize: (panelWidth * 0.05).clamp(16.0, 32.0) // Giảm từ 0.06 và 18-36
                )
              ),
              SizedBox(height: containerPadding * 0.5), // Giảm từ 0.75
              // Wrap GridView trong Flexible để tránh overflow
              Flexible(
                child: GridView.count(
                  shrinkWrap: true,
                  physics: const NeverScrollableScrollPhysics(),
                  crossAxisCount: 3,
                  crossAxisSpacing: cardSpacing,
                  mainAxisSpacing: cardSpacing,
                  childAspectRatio: 1.0, // Card vuông cân đối (1:1)
                  children: List.generate(6, (index) {
                    if (_room != null && index < _room!.members.length) {
                      return _buildPlayerCard(_room!.members[index]);
                    } else {
                      return _buildEmptySlot();
                    }
                  }),
                ),
              ),
              SizedBox(height: containerPadding * 0.5), // Giảm từ 0.75
              Text(
                "PLAYERS (${_room?.members.length ?? 0}/6)", 
                style: TextStyle(
                  fontFamily: 'LuckiestGuy', 
                  color: Colors.white, 
                  fontSize: (panelWidth * 0.035).clamp(12.0, 18.0) // Giảm từ 0.04 và 14-20
                )
              ),
            ],
          ),
        );
      }
    );
  }

  Widget _buildPlayerCard(RoomMember member) {
    final isHostMember = member.accountId == _room!.hostId;
    final statusColor = member.ready ? const Color(0xFF8BC34A) : const Color(0xFFFFB74D);
    // Màu viền: vàng cho host, màu status cho người chơi thường
    final borderColor = isHostMember 
        ? const Color(0xFFFFD700) // Vàng cho host
        : (member.ready ? statusColor : Colors.white24);

    return LayoutBuilder(
      builder: (context, constraints) {
        final w = constraints.maxWidth;
        final h = constraints.maxHeight;
        // Tăng size để các thành phần to hơn, dễ nhìn hơn
        final double avatarRadius = (w * 0.32).clamp(30.0, 65.0); // Tăng từ 0.29 lên 0.32
        final double nameFontSize = (w * 0.32).clamp(14.0, 26.0); // Tăng từ 0.20
        final double statusFontSize = (w * 0.32).clamp(11.0, 20.0); // Tăng từ 0.2

        return Container(
          decoration: BoxDecoration(
            color: Colors.white,
            borderRadius: BorderRadius.circular(12), // Giảm từ 15
            border: Border.all(
              color: borderColor, 
              width: isHostMember ? 4 : 3 // Viền dày hơn cho host
            ),
            // Thêm shadow cho host để nổi bật
            boxShadow: isHostMember ? [
              BoxShadow(
                color: const Color(0xFFFFD700).withOpacity(0.5),
                blurRadius: 8,
                spreadRadius: 2,
              )
            ] : null,
          ),
          child: Stack(
            children: [
              // Dùng Center để content nằm chính giữa card
              Center(
                child: Padding(
                  padding: const EdgeInsets.all(4.0), // Giảm từ 8.0 xuống 4.0
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    mainAxisSize: MainAxisSize.min,
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
                      SizedBox(height: h * 0.03),
                      Flexible(
                        child: Padding(
                          padding: const EdgeInsets.symmetric(horizontal: 3),
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
                      ),
                      SizedBox(height: h * 0.02),
                      Center(
                        child: Container(
                          padding: const EdgeInsets.symmetric(vertical: 4, horizontal: 14),
                          decoration: BoxDecoration(
                            color: statusColor,
                            borderRadius: BorderRadius.circular(5),
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
                      ),
                    ],
                  ),
                ),
              ),
              // Nút KICK ở góc phải trên (tăng size lên)
              if (_isHost && !isHostMember)
                Positioned(
                  top: 4,
                  right: 4,
                  child: InkWell(
                    onTap: () => _kickMember(member.accountId),
                    child: Container(
                      padding: const EdgeInsets.all(5), // Tăng từ 4
                      decoration: BoxDecoration(
                        color: const Color(0xFFE53935),
                        shape: BoxShape.circle,
                        border: Border.all(color: Colors.white, width: 2), // Tăng từ 1.5
                        boxShadow: [
                          BoxShadow(
                            color: Colors.black.withOpacity(0.3),
                            blurRadius: 4,
                          ),
                        ],
                      ),
                      child: const Icon(
                        Icons.close,
                        color: Colors.white,
                        size: 16, // Tăng từ 14
                      ),
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
    final w = MediaQuery.of(context).size.width;
    final fontSize = (w * 0.02).clamp(16.0, 32.0); // Increased font size more
    final verticalPadding = (w * 0.02).clamp(20.0, 40.0); // Increased padding more

    return SizedBox(
      width: double.infinity,
      child: ElevatedButton(
        style: ElevatedButton.styleFrom(
          backgroundColor: bg,
          side: const BorderSide(color: RoomTheme.primaryDark, width: 2),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
          padding: EdgeInsets.symmetric(vertical: verticalPadding),
        ),
        onPressed: onPressed,
        child: Text(label, style: TextStyle(fontFamily: 'LuckiestGuy', color: textCol, fontSize: fontSize)),
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
