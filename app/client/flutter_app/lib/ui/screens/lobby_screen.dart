import 'dart:async';
import 'package:flutter/material.dart';
import '../widgets/user_card.dart';
import '../widgets/app_title.dart';
import '../widgets/room_card.dart';
import '../widgets/add_friend_dialog.dart';
import '../theme/lobby_theme.dart';
import '../../services/service_locator.dart';
import '../../services/friend_service.dart';
import '../../services/room_service.dart';
import 'dart:convert';
import 'package:shared_preferences/shared_preferences.dart';

import '../widgets/invitation_dialog.dart';

class LobbyScreen extends StatefulWidget {
  const LobbyScreen({super.key});

  @override
  State<LobbyScreen> createState() => _LobbyScreenState();
}

class _LobbyScreenState extends State<LobbyScreen> {
  int _pendingRequests = 0;
  StreamSubscription? _friendSub;
  StreamSubscription? _roomSub;
  // Timer? _pollTimer; // Removed to optimize network

  @override
  void initState() {
    super.initState();
    // Only fetch once on init
    _fetchPendingRequests();
    
    // Check if user needs to be restored to a room
    _checkPendingRoomRestore();
    
    // Listen for friend updates (Real-time instead of polling)
    _friendSub = ServiceLocator.friendService.events.listen((event) {
        if (!mounted) return;
        
        if (event.type == FriendEventType.requestReceived) {
             setState(() {
                 _pendingRequests++;
             });
             _fetchPendingRequests();
        } else if (event.type == FriendEventType.requestAccepted ||
            event.type == FriendEventType.requestRejected ||
            event.type == FriendEventType.friendAdded ||
            event.type == FriendEventType.friendRemoved) {
             _fetchPendingRequests();
        }
    });

    // Listen for room invitations
    _roomSub = ServiceLocator.roomService.events.listen((event) {
        if (!mounted) return;
        if (event.type == RoomEventType.invitationReceived) {
             final data = event.data;
             // Wrap in post frame callback to avoid navigation errors if building
             WidgetsBinding.instance.addPostFrameCallback((_) {
                 if (mounted) {
                   showDialog(
                       context: context,
                       barrierDismissible: false,
                       builder: (context) => InvitationDialog(
                           senderId: data['sender_id'],
                           senderName: data['sender_name'] ?? "Unknown",
                           roomId: data['room_id'],
                           roomName: data['room_name'] ?? "Game Room"
                       )
                   );
                 }
             });
        }
    });
  }

  Future<void> _checkPendingRoomRestore() async {
    final prefs = await SharedPreferences.getInstance();
    final roomDataStr = prefs.getString("pending_room_restore");
    
    if (roomDataStr != null) {
      try {
        final roomData = jsonDecode(roomDataStr);
        final roomId = roomData["room_id"];
        
        print("🏠 Restoring room: ${roomData["room_name"]} (ID: $roomId)");
        
        // Clear pending restore
        await prefs.remove("pending_room_restore");
        
        // Fetch full room data and navigate
        WidgetsBinding.instance.addPostFrameCallback((_) async {
          if (!mounted) return;
          
          try {
            // Join the room to get full data
            final room = await ServiceLocator.roomService.joinRoom(roomId);
            if (room != null && mounted) {
              Navigator.pushNamed(
                context, 
                '/room', 
                arguments: {
                  'room': room,
                  'initialIsHost': roomData["is_host"] == true
                }
              );
            }
          } catch (e) {
            print("❌ Failed to restore room: $e");
          }
        });
      } catch (e) {
        print("❌ Error parsing pending room: $e");
        await prefs.remove("pending_room_restore");
      }
    }
  }

  Future<void> _fetchPendingRequests() async {
      try {
          final res = await ServiceLocator.friendService.getFriendRequests();
          if (mounted && res["success"] == true) {
              setState(() {
                  _pendingRequests = (res["count"] ?? 0) as int;
              });
          }
      } catch (_) {}
  }

  @override
  void dispose() {
      _friendSub?.cancel();
      _roomSub?.cancel();
      super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final horizontalPadding = screenWidth > 1200 ? 100.0 : 40.0;

    final screenHeight = MediaQuery.of(context).size.height;

    return Scaffold(
      backgroundColor: LobbyTheme.backgroundNavy,
      body: Stack(
        children: [
          // Background Image
          Positioned.fill(
            child: Image.asset("assets/images/lobby-bg.png", fit: BoxFit.cover),
          ),
          Positioned.fill(child: Container(color: Colors.black.withOpacity(0.5))),

          // 1. User Card & Add Friend Button (Góc trên bên trái)
          Positioned(
            top: screenHeight * 0.04,
            left: screenWidth * 0.02,
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.center,
              children: [
                const UserCard(),
                const SizedBox(width: 12),
                Stack(
                  clipBehavior: Clip.none,
                  children: [
                      Container(
                         width: 72,
                         height: 72,
                         decoration: BoxDecoration(
                           color: LobbyTheme.yellowGame,
                           shape: BoxShape.circle,
                           border: Border.all(color: LobbyTheme.primaryDark, width: 3),
                           boxShadow: [
                             BoxShadow(
                               color: Colors.black.withOpacity(0.3),
                               offset: const Offset(0, 4),
                               blurRadius: 8,
                             )
                           ],
                         ),
                         child: IconButton(
                           icon: const Icon(Icons.person_add, color: LobbyTheme.primaryDark),
                           iconSize: 32,
                           onPressed: () {
                              showDialog(
                                context: context,
                                builder: (context) => AddFriendDialog(initialIndex: _pendingRequests > 0 ? 1 : 0),
                              ).then((_) => _fetchPendingRequests());
                           },
                           tooltip: "Add Friend",
                         ),
                      ),
                      if (_pendingRequests > 0)
                         Positioned(
                           right: -5,
                           top: -5,
                           child: Container(
                             padding: const EdgeInsets.all(6),
                             decoration: const BoxDecoration(
                               color: Colors.red, 
                               shape: BoxShape.circle,
                               boxShadow: [BoxShadow(color: Colors.black26, blurRadius: 2)]
                             ),
                             child: Text(
                               "$_pendingRequests", 
                               style: const TextStyle(color: Colors.white, fontSize: 12, fontWeight: FontWeight.bold)
                             ),
                           ),
                         )
                  ],
                ),
              ],
            ),
          ),

          SafeArea(
            child: Column(
              children: [
                SizedBox(height: screenHeight * 0.04), // Reduced from 0.06
                const AppTitle(), // Title in center
                Expanded(
                  child: Center(
                    child: ConstrainedBox(
                      // Giới hạn tổng width của content để không bị quá rộng
                      constraints: const BoxConstraints(
                        maxWidth: 1400, // Tổng width tối đa cho toàn bộ content
                      ),
                      child: Padding(
                        padding: EdgeInsets.symmetric(
                          horizontal: 40,
                          vertical: screenHeight * 0.03,
                        ),
                        child: Row(
                          crossAxisAlignment: CrossAxisAlignment.center,
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            // Left column: Action buttons (width cố định)
                            SizedBox(
                              width: 280, // Tăng từ 250 lên 280
                              child: Column(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  _sideButton(context, "VIEW HISTORY", () {
                                    Navigator.pushNamed(context, '/history');
                                  }),
                                  _sideButton(context, "VIEW TUTORIAL", () {
                                    Navigator.pushNamed(context, '/tutorial');
                                  }),
                                  _sideButton(context, "SETTING", () async {
                                    await Navigator.pushNamed(context, '/settings');
                                    // Settings pop doesn't return value but we want to trigger update
                                    // ProfileService stream will handle this
                                  }),
                                  const SizedBox(height: 16),
                                  _sideButton(context, "LOG OUT", () async {
                                    final res = await ServiceLocator.authService.logout();
                                    if (res["success"] == true) {
                                      if (context.mounted) {
                                        Navigator.pushReplacementNamed(context, '/login');
                                      }
                                    } else {
                                      if (context.mounted) {
                                        ScaffoldMessenger.of(context).showSnackBar(
                                          SnackBar(content: Text(res["error"] ?? "Logout failed")),
                                        );
                                      }
                                    }
                                  }, isLogout: true),
                                ],
                              ),
                            ),
                            
                            const SizedBox(width: 40), // Khoảng cách cố định
                            
                            // Right column: Room Panel (giới hạn width chặt chẽ hơn)
                            Flexible(
                              child: ConstrainedBox(
                                constraints: const BoxConstraints(
                                  maxWidth: 800, // Giảm từ 1100 xuống 800
                                  maxHeight: 550, // Thêm giới hạn chiều cao
                                ),
                                child: const RoomCard(),
                              ),
                            ),
                          ],
                        ),
                      ),
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

  Widget _sideButton(BuildContext context, String label, VoidCallback onPressed, {bool isLogout = false}) {
    // Calculate responsive font size based on screen width
    // Increased size as requested
    final screenWidth = MediaQuery.of(context).size.width;
    final double fontSize = (screenWidth * 0.02).clamp(18.0, 32.0);
    final double verticalPadding = (screenWidth * 0.015).clamp(14.0, 26.0); // Tăng từ 12-24 lên 14-26
    final double horizontalPadding = (screenWidth * 0.008).clamp(12.0, 24.0); // Giảm từ 20-40 xuống 12-24

    return Container(
      margin: const EdgeInsets.only(bottom: 16), // Fixed margin for consistent spacing
      width: double.infinity,
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(16),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.25),
            offset: const Offset(0, 6),
            blurRadius: 16,
          )
        ],
      ),
      child: ElevatedButton(
        style: isLogout ? LobbyTheme.logoutButtonStyle : LobbyTheme.sideButtonStyle,
        onPressed: onPressed,
        child: Padding(
          padding: EdgeInsets.symmetric(
            vertical: verticalPadding, 
            horizontal: horizontalPadding, // Thêm padding ngang
          ),
          child: Text(
            label,
            style: LobbyTheme.gameFont(fontSize: fontSize), 
          ),
        ),
      ),
    );
  }
}