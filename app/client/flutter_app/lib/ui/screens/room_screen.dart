import 'dart:async';
import 'package:flutter/material.dart';
import '../../services/service_locator.dart';
import '../../services/room_service.dart';
import '../../services/game_service.dart';
import '../../models/room.dart';
import '../theme/lobby_theme.dart';
import '../theme/room_theme.dart';
import '../widgets/user_card.dart';
import '../widgets/invite_players_dialog.dart';

class RoomScreen extends StatefulWidget {
  final Room room;
  final bool initialIsHost;

  const RoomScreen({super.key, required this.room, this.initialIsHost = false});

  @override
  State<RoomScreen> createState() => _RoomScreenState();
}

class _RoomScreenState extends State<RoomScreen> {
  late Room _room;
  bool _isLoading = false;
  String? _error;
  int? _currentUserId;
  StreamSubscription? _eventSub;
  bool _editMode = false;
  
  // Temp rules for edit mode
  String _tempMode = "scoring";
  int _tempMaxPlayers = 6;
  String _tempVisibility = "public";
  bool _tempWager = true;

  bool get _isHost => _room.hostId == _currentUserId;

  @override
  void initState() {
    super.initState();
    _room = widget.room;
    // Init temp with current
    _tempMode = _room.mode;
    _tempMaxPlayers = _room.maxPlayers;
    _tempVisibility = _room.visibility;
    _tempWager = _room.wagerMode;
    _init();
  }

  // ... (rest)

  Future<void> _saveRules() async {
      try {
          await ServiceLocator.roomService.setRules(
              mode: _tempMode,
              maxPlayers: _tempMaxPlayers,
              visibility: _tempVisibility,
              wager: _tempWager
          );
          ScaffoldMessenger.of(context).showSnackBar(
             const SnackBar(content: Text("Rules updated!"))
          );
      } catch (e) {
          if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("Failed to save rules: $e")));
      }
  }

  // ... dispose ...

  Future<void> _init() async {
    final authState = await ServiceLocator.authService.getAuthState();
    if (authState["accountId"] != null) {
      setState(() {
          _currentUserId = int.tryParse(authState["accountId"]!);
      });
    }
    _eventSub = ServiceLocator.roomService.events.listen(_handleEvent);
  }

  // Remove _fetchRoom entirely

  void _handleEvent(RoomEvent event) {
    if (!mounted) return;
    
    setState(() {
        switch (event.type) {
            case RoomEventType.playerJoined:
                // data is Map<String, dynamic> from ntfPlayerJoined
                // logic to add player
                final p = event.data;
                if (p != null) {
                    // Check duplicate
                    if (!_room.members.any((m) => m.accountId == p['account_id'])) {
                        _room.members.add(RoomMember(
                            accountId: p['account_id'],
                            email: p['name'] ?? "Unknown",
                            avatar: null, // Payload might not have avatar?
                            ready: false
                        ));
                        // Update player count if needed (Room model might need updating to be mutable or copyWith)
                        // For now we modify list in place, but Room has final fields for counts.
                        // Ideally we use copyWith.
                    }
                }
                break;
            case RoomEventType.playerLeft:
                final p = event.data;
                 if (p != null) {
                      _room.members.removeWhere((m) => m.accountId == p['account_id']);
                 }
                break;
            case RoomEventType.playerReady:
                final data = event.data;
                if (data != null) {
                    final accountId = data['account_id'];
                    final isReady = data['is_ready'] == true;
                    final idx = _room.members.indexWhere((m) => m.accountId == accountId);
                    if (idx != -1) {
                         _room.members[idx] = _room.members[idx].copyWith(ready: isReady);
                    }
                }
                break;
            case RoomEventType.rulesChanged:
                final data = event.data;
                if (data != null) {
                    _room = _room.copyWith(
                        mode: data['mode'] ?? "scoring",
                        maxPlayers: data['maxPlayers'] ?? 6,
                        visibility: data['visibility'] ?? "public",
                        wagerMode: data['wagerMode'] == true,
                    );
                    // Server also resets ready states on rule change
                    for (int i = 0; i < _room.members.length; i++) {
                        _room.members[i] = _room.members[i].copyWith(ready: false);
                    }
                }
                break;
            case RoomEventType.hostChanged:
                final data = event.data;
                if (data != null) {
                    final newHostId = data['host_id'] ?? data['hostId'] ?? data['new_host_id'];
                    if (newHostId != null) {
                        _room = _room.copyWith(hostId: newHostId);
                        
                        if (newHostId == _currentUserId) {
                             ScaffoldMessenger.of(context).showSnackBar(
                               const SnackBar(content: Text("You are now the host!"))
                             );
                        }
                    }
                }
                break;
            default:
                break;
        }
    });

    switch (event.type) {
      // ... keep toast/dialog logic which is separate from state update ...
      case RoomEventType.roomClosed:
        _showExitDialog("Room Closed", "The host has closed the room.");
        break;
      case RoomEventType.memberKicked:
        _showExitDialog("Kicked", "You have been kicked from the room.");
        break;
      case RoomEventType.gameStarted:
        // Navigate to the Game Container Screen
        print("[RoomScreen] 🎮 Game started event received!");
        final data = event.data as Map<String, dynamic>?;
        print("[RoomScreen] Event data: $data");
        final matchId = data?['match_id'] ?? 1; // Default to 1 if missing for testing
        print("[RoomScreen] Navigating to /game with matchId: $matchId");
        if (mounted) {
          Navigator.pushNamed(context, '/game', arguments: {
            'room': _room,
            'matchId': matchId,
          });
        }
        break;
      default: break;
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
    await ServiceLocator.roomService.leaveRoom(_room.id);
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
      await ServiceLocator.roomService.kickMember(_room.id, targetId);
    }
  }

  Future<void> _toggleReady() async {
      try {
          await ServiceLocator.roomService.toggleReady();
      } catch (e) {
          if (mounted) {
              ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(content: Text("Error: $e")),
              );
          }
      }
  }

  Future<void> _showEditRulesDialog() async {
    String currentMode = _room.mode;
    int currentMaxPlayers = _room.maxPlayers;
    String currentVisibility = _room.visibility;
    bool currentWager = _room.wagerMode;

    await showDialog(
      context: context,
      builder: (context) => StatefulBuilder(
        builder: (context, setDialogState) {
          return AlertDialog(
            title: const Text("Edit Game Rules"),
            content: SingleChildScrollView(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  DropdownButtonFormField<String>(
                    decoration: const InputDecoration(labelText: "Game Mode"),
                    value: currentMode.toLowerCase(),
                    items: const [
                      DropdownMenuItem(value: "elimination", child: Text("Elimination (4 Players)")),
                      DropdownMenuItem(value: "scoring", child: Text("Scoring (4-6 Players)")),
                    ],
                    onChanged: (val) {
                      if (val != null) {
                         setDialogState(() {
                           currentMode = val;
                           if (val == "elimination") currentMaxPlayers = 4;
                         });
                      }
                    },
                  ),
                  if (currentMode == "scoring")
                    TextFormField(
                      decoration: const InputDecoration(labelText: "Max Players (4-6)"),
                      initialValue: currentMaxPlayers.toString(),
                      keyboardType: TextInputType.number,
                      onChanged: (val) => currentMaxPlayers = int.tryParse(val) ?? 6,
                    ),
                  DropdownButtonFormField<String>(
                    decoration: const InputDecoration(labelText: "Visibility"),
                    value: currentVisibility.toLowerCase(),
                    items: const [
                      DropdownMenuItem(value: "public", child: Text("Public")),
                      DropdownMenuItem(value: "private", child: Text("Private")),
                    ],
                    onChanged: (val) => setDialogState(() => currentVisibility = val ?? "public"),
                  ),
                  SwitchListTile(
                    title: const Text("Wager Mode"),
                    value: currentWager,
                    onChanged: (val) => setDialogState(() => currentWager = val),
                  ),
                ],
              ),
            ),
            actions: [
              TextButton(onPressed: () => Navigator.pop(context), child: const Text("Cancel")),
              ElevatedButton(
                onPressed: () async {
                  try {
                    await ServiceLocator.roomService.setRules(
                      mode: currentMode,
                      maxPlayers: currentMaxPlayers,
                      visibility: currentVisibility,
                      wager: currentWager,
                    );
                    if (context.mounted) Navigator.pop(context);
                  } catch (e) {
                    if (context.mounted) {
                      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("$e")));
                    }
                  }
                },
                child: const Text("Save"),
              ),
            ],
          );
        }
      ),
    );
  }
  
  Future<void> _startGame() async {
    final result = await ServiceLocator.gameService.startGame(_room.id);
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
    // Current display values
    final dMode = _editMode ? _tempMode : _room.mode;
    final dMax = _editMode ? _tempMaxPlayers : _room.maxPlayers;
    final dVis = _editMode ? _tempVisibility : _room.visibility;
    final dWager = _editMode ? _tempWager : _room.wagerMode;

    return LayoutBuilder(
      builder: (context, constraints) {
        final w = constraints.maxWidth;
        final titleSize = (w * 0.1).clamp(20.0, 32.0);
        
        return Container(
          decoration: BoxDecoration(
            color: const Color(0xFF1F2A44).withOpacity(0.9),
            borderRadius: BorderRadius.circular(18),
            border: Border.all(color: const Color(0xFF1B2333), width: 4),
            boxShadow: [
              BoxShadow(
                color: Colors.black.withOpacity(0.4),
                blurRadius: 20,
                offset: const Offset(0, 8),
              )
            ],
          ),
          padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 25),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // Header Row with EDIT/DONE
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                crossAxisAlignment: CrossAxisAlignment.baseline,
                textBaseline: TextBaseline.alphabetic,
                children: [
                  Flexible(
                    child: Text("GAME RULES", 
                      style: TextStyle(
                        fontFamily: 'LuckiestGuy', 
                        fontSize: (titleSize * 0.8).clamp(18.0, 28.0), 
                        color: const Color(0xFFFFEB3B),
                        shadows: const [Shadow(offset: Offset(2, 2), color: Color(0xFF1B2333))]
                      ),
                      overflow: TextOverflow.ellipsis,
                    ),
                  ),
                  if (_isHost) ...[
                    const SizedBox(width: 8),
                    GestureDetector(
                      onTap: () async {
                        if (_editMode) {
                            // DONE -> Save
                            await _saveRules();
                            setState(() => _editMode = false);
                        } else {
                            // EDIT -> Init temp
                            setState(() {
                                _tempMode = _room.mode;
                                _tempMaxPlayers = _room.maxPlayers;
                                _tempVisibility = _room.visibility;
                                _tempWager = _room.wagerMode;
                                _editMode = true;
                            });
                        }
                      },
                      child: Text(_editMode ? "DONE" : "EDIT", 
                        style: TextStyle(
                          fontFamily: 'LuckiestGuy', 
                          fontSize: 12, 
                          color: _editMode ? Colors.greenAccent : Colors.white.withOpacity(0.4),
                          fontStyle: FontStyle.italic,
                          letterSpacing: 1,
                        )
                      ),
                    ),
                  ],
                ],
              ),
              const SizedBox(height: 25),
              
              // Max Players
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  const Text("MAX PLAYERS:", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 18, color: Color(0xFF29B6F6))),
                  if (_isHost && _editMode)
                    Row(
                      children: [
                        _controlBtn("-", () {
                            if (dMode.toLowerCase() != "elimination" && dMax > 4) {
                                setState(() => _tempMaxPlayers--);
                            }
                        }, enabled: dMode.toLowerCase() != "elimination" && dMax > 4),
                        Padding(
                          padding: const EdgeInsets.symmetric(horizontal: 12),
                          child: Text("$dMax", style: const TextStyle(fontFamily: 'LuckiestGuy', fontSize: 28, color: Color(0xFFFFEB3B))),
                        ),
                        _controlBtn("+", () {
                            if (dMode.toLowerCase() != "elimination" && dMax < 6) {
                                setState(() => _tempMaxPlayers++);
                            }
                        }, enabled: dMode.toLowerCase() != "elimination" && dMax < 6),
                      ],
                    )
                  else
                    Text("$dMax", style: const TextStyle(fontFamily: 'LuckiestGuy', fontSize: 28, color: Color(0xFFFFEB3B))),
                ],
              ),
              const SizedBox(height: 20),

              // Visibility
              const Text("VISIBILITY:", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 18, color: Color(0xFF29B6F6))),
              const SizedBox(height: 10),
              Row(
                children: [
                   _wrToggleButton("PUBLIC", dVis.toLowerCase() == "public", 
                       () => setState(() => _tempVisibility = "public")),
                   const SizedBox(width: 10),
                   _wrToggleButton("PRIVATE", dVis.toLowerCase() == "private", 
                       () => setState(() => _tempVisibility = "private")),
                ],
              ),
              const SizedBox(height: 20),

              // Mode
              const Text("MODE:", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 18, color: Color(0xFF29B6F6))),
              const SizedBox(height: 10),
              Row(
                children: [
                   _wrToggleButton("ELIMINATION", dMode.toLowerCase() == "elimination", 
                       () => setState(() { 
                           _tempMode = "elimination"; 
                           _tempMaxPlayers = 4; 
                       })),
                   const SizedBox(width: 10),
                   _wrToggleButton("SCORING", dMode.toLowerCase() == "scoring", 
                       () => setState(() => _tempMode = "scoring")),
                ],
              ),
              const SizedBox(height: 20),

              // Wager
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  const Flexible(child: Text("WAGER MODE:", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 16, color: Color(0xFF29B6F6)))),
                  const SizedBox(width: 4),
                  Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                       _wagerBadge("ON", dWager, () => setState(() => _tempWager = true)),
                       const SizedBox(width: 8),
                       _wagerBadge("OFF", !dWager, () => setState(() => _tempWager = false)),
                    ],
                  ),
                ],
              ),
            ],
          ),
        );
      }
    );
  }

  Widget _controlBtn(String label, VoidCallback onTap, {bool enabled = true}) {
      return GestureDetector(
          onTap: enabled ? onTap : null,
          child: Container(
              width: 32,
              height: 32,
              decoration: BoxDecoration(
                  color: const Color(0xFF2c3e50),
                  borderRadius: BorderRadius.circular(4),
                  border: Border.all(color: const Color(0xFF1B2333), width: 2),
              ),
              alignment: Alignment.center,
              child: Opacity(
                  opacity: enabled ? 1.0 : 0.3,
                  child: Text(label, style: const TextStyle(color: Colors.white, fontSize: 20, fontWeight: FontWeight.bold)),
              ),
          ),
      );
  }

  Widget _wrToggleButton(String label, bool isActive, VoidCallback onTap) {
    return Expanded(
      child: GestureDetector(
        onTap: (_isHost && _editMode) ? onTap : null,
        child: Container(
          padding: const EdgeInsets.symmetric(vertical: 12),
          decoration: BoxDecoration(
            color: isActive ? const Color(0xFF8BC34A) : const Color(0xFF2c3e50),
            borderRadius: BorderRadius.circular(10),
            border: Border.all(color: const Color(0xFF1B2333), width: 2),
            boxShadow: isActive ? [
              BoxShadow(color: const Color(0xFF558b2f), offset: const Offset(0, 4))
            ] : null,
          ),
          alignment: Alignment.center,
          child: Opacity(
            opacity: (_isHost && _editMode) || isActive ? 1.0 : 0.6,
            child: Text(label, style: TextStyle(
              fontFamily: 'LuckiestGuy', 
              fontSize: 16, 
              color: isActive ? Colors.white : const Color(0xFF5d6d7e),
              shadows: isActive ? [const Shadow(offset: Offset(1, 1), color: Color(0xFF1B2333))] : null,
            )),
          ),
        ),
      ),
    );
  }

  Widget _wagerBadge(String label, bool isActive, VoidCallback onTap) {
      final activeColor = label == "ON" ? const Color(0xFF8BC34A) : const Color(0xFFf44336);
      return GestureDetector(
          onTap: (_isHost && _editMode) ? onTap : null,
          child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 15, vertical: 6),
              constraints: const BoxConstraints(minWidth: 55),
              decoration: BoxDecoration(
                  color: isActive ? activeColor : const Color(0xFF2c3e50),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(color: isActive ? Colors.white : const Color(0xFF1B2333), width: 2),
                  boxShadow: isActive ? [
                      BoxShadow(color: label == "ON" ? const Color(0xFF558b2f) : const Color(0xFFc62828), offset: const Offset(0, 4))
                  ] : null,
              ),
              alignment: Alignment.center,
              child: Opacity(
                  opacity: (_isHost && _editMode) || isActive ? 1.0 : 0.6,
                  child: Text(label, style: TextStyle(
                      fontFamily: 'LuckiestGuy', 
                      fontSize: 16, 
                      color: isActive ? Colors.white : const Color(0xFF5d6d7e),
                      shadows: isActive ? [const Shadow(offset: Offset(1, 1), color: Color(0xFF1B2333))] : null,
                  )),
              ),
          ),
      );
  }

  void _handleMaxPlayersChange(int delta) {
      int newVal = _room.maxPlayers + delta;
      if (newVal >= 4 && newVal <= 6) {
          _updateRule(maxPlayers: newVal);
      }
  }

  Future<void> _updateRule({String? mode, int? maxPlayers, String? visibility, bool? wager}) async {
    if (!_isHost) return;
    
    // Logic: Nếu chọn elimination -> Mặc định là 4 người
    int finalMaxPlayers = maxPlayers ?? _room.maxPlayers;
    String finalMode = mode ?? _room.mode;
    
    if (mode == "elimination") {
        finalMaxPlayers = 4;
    } else if (mode == "scoring") {
        if (finalMaxPlayers < 4) finalMaxPlayers = 4;
        if (finalMaxPlayers > 6) finalMaxPlayers = 6;
    }

    try {
      await ServiceLocator.roomService.setRules(
        mode: finalMode,
        maxPlayers: finalMaxPlayers,
        visibility: visibility ?? _room.visibility,
        wager: wager ?? _room.wagerMode,
      );
      // Room state will be updated via NTF_RULES_CHANGED
    } catch (e) {
      if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("$e")));
    }
  }

  Widget _buildMemberListPanel() {
    return LayoutBuilder(
      builder: (context, constraints) {
        double panelWidth = constraints.maxWidth;
        double cardSpacing = 15.0;
        
        return Container(
          decoration: RoomTheme.panelDecoration.copyWith(
            color: const Color(0xFF232D42),
          ),
          padding: const EdgeInsets.all(20),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              // Room Code Badge
              Align(
                alignment: Alignment.centerLeft,
                child: Container(
                  padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                  decoration: BoxDecoration(
                    color: const Color(0xFF1B2333),
                    borderRadius: BorderRadius.circular(5),
                  ),
                  child: Text("ID: ${_room.code ?? _room.id}", 
                    style: const TextStyle(color: Colors.white54, fontSize: 12, fontWeight: FontWeight.bold)),
                ),
              ),
              Text(
                _room.name.toUpperCase(), 
                style: const TextStyle(
                  fontFamily: 'LuckiestGuy', 
                  fontSize: 28, 
                  color: Color(0xFFFFDE00)
                ),
                textAlign: TextAlign.center,
              ),
              const SizedBox(height: 15),
              Flexible(
                child: GridView.count(
                  shrinkWrap: true,
                  physics: const NeverScrollableScrollPhysics(),
                  crossAxisCount: 3,
                  crossAxisSpacing: cardSpacing,
                  mainAxisSpacing: cardSpacing,
                  childAspectRatio: 0.85, 
                  children: List.generate(6, (index) {
                    if (index < _room.members.length) {
                      return _buildPlayerCard(_room.members[index]);
                    } else {
                      return _buildEmptySlot();
                    }
                  }),
                ),
              ),
              const SizedBox(height: 20),
              Text(
                "PLAYERS (${_room.members.length}/6)", 
                style: const TextStyle(
                  fontFamily: 'LuckiestGuy', 
                  color: Color(0xFFFFDE00), 
                  fontSize: 16
                )
              ),
            ],
          ),
        );
      }
    );
  }

  Widget _buildPlayerCard(RoomMember member) {
    final isHostMember = member.accountId == _room.hostId;
    final statusColor = member.ready ? const Color(0xFF8BC34A) : const Color(0xFFFFC107);
    
    return Container(
      decoration: BoxDecoration(
        color: const Color(0xFFF0F0F0),
        borderRadius: BorderRadius.circular(15),
        border: Border.all(
          color: isHostMember ? const Color(0xFFFFDE00) : Colors.white70, 
          width: 3
        ),
      ),
      child: Stack(
        clipBehavior: Clip.none,
        children: [
          if (isHostMember)
            Positioned(
              top: -15,
              right: -10,
              child: Transform.rotate(
                angle: 0.3,
                child: const Icon(Icons.workspace_premium, color: Colors.orange, size: 36),
              ),
            ),
          
          // Kick button for host (only if not clicking themselves)
          if (_isHost && !isHostMember)
            Positioned(
              top: -5,
              left: -5,
              child: GestureDetector(
                onTap: () => _kickMember(member.accountId),
                child: Container(
                  padding: const EdgeInsets.all(4),
                  decoration: const BoxDecoration(
                    color: Colors.redAccent,
                    shape: BoxShape.circle,
                  ),
                  child: const Icon(Icons.close, color: Colors.white, size: 14),
                ),
              ),
            ),

          Center(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                CircleAvatar(
                  radius: 30,
                  backgroundColor: Colors.white,
                  backgroundImage: (member.avatar != null && member.avatar!.isNotEmpty)
                      ? (member.avatar!.startsWith("http") ? NetworkImage(member.avatar!) : AssetImage(member.avatar!) as ImageProvider)
                      : const AssetImage('assets/images/default-mushroom.jpg'),
                ),
                const SizedBox(height: 8),
                Text(member.email.toUpperCase(), 
                  style: const TextStyle(fontFamily: 'LuckiestGuy', fontSize: 14, color: Color(0xFF1B2333)),
                  overflow: TextOverflow.ellipsis,
                ),
                const SizedBox(height: 5),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 2),
                  decoration: BoxDecoration(
                    color: statusColor,
                    borderRadius: BorderRadius.circular(10),
                  ),
                  child: Text(member.ready ? "READY" : "WAITING", 
                    style: const TextStyle(fontFamily: 'LuckiestGuy', fontSize: 10, color: Colors.white)),
                )
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildEmptySlot() {
    return Container(
      decoration: BoxDecoration(
        color: const Color(0xFF1B2333).withOpacity(0.4),
        borderRadius: BorderRadius.circular(15),
        border: Border.all(color: Colors.white10, width: 2, style: BorderStyle.none),
      ),
      child: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.help, color: Colors.white.withOpacity(0.1), size: 40),
            const SizedBox(height: 5),
            Text("EMPTY", style: TextStyle(fontFamily: 'LuckiestGuy', color: Colors.white.withOpacity(0.1), fontSize: 14)),
          ],
        ),
      ),
    );
  }

  Widget _buildActionButtons() {
    final me = _room.members.firstWhere((m) => m.accountId == _currentUserId, 
        orElse: () => RoomMember(accountId: -1, email: ""));
    final myReady = me.ready;

    // Check if everyone is ready (including Host)
    final allReady = _room.members.length >= 1 && _room.members.every((m) => m.ready);

    return Column(
      children: [
        // Ready Button for EVERYONE
        _actionButton(
          myReady ? "NOT READY" : "READY", 
          myReady ? RoomTheme.accentRed : RoomTheme.accentGreen, 
          Colors.white, 
          _toggleReady
        ),
        const SizedBox(height: 10),

        _actionButton("INVITE FRIENDS", RoomTheme.accentGreen, Colors.white, () {
          showDialog(
            context: context,
            builder: (context) => InvitePlayersDialog(roomId: _room.id),
          );
        }),
        const SizedBox(height: 10),
        _actionButton("LEAVE ROOM", RoomTheme.accentRed, Colors.white, _leaveRoom),

        if (_isHost) ...[
          const SizedBox(height: 10),
          // Start Game button only for Host at the bottom
          Opacity(
            opacity: allReady ? 1.0 : 0.5,
            child: _actionButton(
              "START GAME", 
              RoomTheme.accentYellow, 
              RoomTheme.primaryDark, 
              () {
                if (allReady) {
                  _startGame();
                } else {
                  String msg = _room.members.length < 1 
                      ? "Need at least 1 player to start!" 
                      : "All players must be READY to start!";
                  ScaffoldMessenger.of(context).showSnackBar(
                    SnackBar(content: Text(msg)),
                  );
                }
              }
            ),
          ),
        ],
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
