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
    return Container(
      decoration: RoomTheme.panelDecoration,
      padding: const EdgeInsets.all(15),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          const Text("GAME RULES", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 22, color: Color(0xFFFFEB3B))),
          const SizedBox(height: 15),
          _ruleRow("MAX PLAYERS", "6"), // Cập nhật đúng số lượng 6
          _ruleRow("VISIBILITY", "PUBLIC"),
          _ruleRow("MODE", "SCORING"),
          _ruleRow("ROUND TIME", "15S"),
          _ruleRow("WAGER", "ON"),
        ],
      ),
    );
  }

  Widget _buildMemberListPanel() {
    return Container(
      decoration: RoomTheme.panelDecoration,
      padding: const EdgeInsets.all(15), // Giảm padding để tăng diện tích hiển thị
      child: Column(
        children: [
          Text(_room?.name ?? "ROOM", style: RoomTheme.titleStyle),
          const SizedBox(height: 15),
          Expanded(
            child: GridView.count(
              crossAxisCount: 3,
              crossAxisSpacing: 12,
              mainAxisSpacing: 12,
              // Chỉnh từ 0.7 lên 1.0 hoặc 1.1 để card ngắn lại (bé hơn)
              childAspectRatio: 1.0, 
              physics: const NeverScrollableScrollPhysics(),
              children: List.generate(6, (index) {
                if (_room != null && index < _room!.members.length) {
                  return _buildPlayerCard(_room!.members[index]);
                } else {
                  return _buildEmptySlot();
                }
              }),
            ),
          ),
          Text(
            "PLAYERS (${_room?.members.length ?? 0}/6)", 
            style: const TextStyle(fontFamily: 'LuckiestGuy', color: Colors.white70, fontSize: 14)
          ),
        ],
      ),
    );
  }

  Widget _buildPlayerCard(RoomMember member) {
    final isHostMember = member.accountId == _room!.hostId;
    final statusColor = member.ready ? const Color(0xFF8BC34A) : const Color(0xFFFFB74D);

    return Container(
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(
          color: member.ready ? statusColor : Colors.white24, 
          width: 2 // Giảm độ dày viền
        ),
      ),
      child: Stack(
        children: [
          Column(
            mainAxisAlignment: MainAxisAlignment.center, // Căn giữa nội dung
            children: [
              CircleAvatar(
                radius: 20, // Thu nhỏ avatar (từ 25 xuống 20)
                backgroundColor: const Color(0xFFE0E0E0),
                // Sử dụng ảnh nấm nếu không có avatar
                backgroundImage: member.avatar != null 
                  ? NetworkImage(member.avatar!) 
                  : const AssetImage('assets/images/mushroom.png') as ImageProvider,
                child: member.avatar == null 
                  ? const SizedBox.shrink() // Ẩn icon người đi vì đã có nấm
                  : null, 
              ),
              const SizedBox(height: 4),
              Text(
                member.email.toUpperCase(),
                style: const TextStyle(
                  fontFamily: 'LuckiestGuy',
                  fontSize: 12, // Giảm cỡ chữ tên
                  color: Color(0xFF1F2A44),
                ),
                overflow: TextOverflow.ellipsis,
              ),
              const SizedBox(height: 4),
              // Thanh trạng thái nhỏ gọn phía dưới
              Container(
                width: double.infinity,
                margin: const EdgeInsets.symmetric(horizontal: 10),
                padding: const EdgeInsets.symmetric(vertical: 2),
                decoration: BoxDecoration(
                  color: statusColor,
                  borderRadius: BorderRadius.circular(4),
                ),
                child: Text(
                  member.ready ? "READY" : "WAITING",
                  textAlign: TextAlign.center,
                  style: const TextStyle(
                    fontFamily: 'LuckiestGuy',
                    fontSize: 9, // Chữ trạng thái nhỏ lại
                    color: Colors.white,
                  ),
                ),
              ),
            ],
          ),
          if (isHostMember)
            const Positioned(top: 4, left: 4, child: Icon(Icons.star, color: Colors.amber, size: 16)),
        ],
      ),
    );
  }

  Widget _buildEmptySlot() {
    return Container(
      decoration: BoxDecoration(
        color: Colors.black.withOpacity(0.3), // Tối hơn một chút để phân biệt
        borderRadius: BorderRadius.circular(15),
        border: Border.all(color: Colors.white10, width: 2),
      ),
      child: const Center(
        child: Text(
          "EMPTY",
          style: TextStyle(
            fontFamily: 'LuckiestGuy',
            color: Colors.white24,
            fontSize: 14,
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

  Widget _ruleRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(fontFamily: 'LuckiestGuy', color: Color(0xFF29B6F6), fontSize: 18)),
          Text(value, style: const TextStyle(fontFamily: 'LuckiestGuy', color: RoomTheme.accentYellow, fontSize: 22)),
        ],
      ),
    );
  }
}