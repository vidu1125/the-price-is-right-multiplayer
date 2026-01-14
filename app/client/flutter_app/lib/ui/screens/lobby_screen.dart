import 'package:flutter/material.dart';
import '../widgets/user_card.dart';
import '../widgets/app_title.dart';
import '../widgets/room_card.dart';
import '../theme/lobby_theme.dart';
import '../../services/service_locator.dart';

class LobbyScreen extends StatelessWidget {
  const LobbyScreen({super.key});

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

          // 1. User Card (Góc trên bên trái - Responsive positioning)
          Positioned(
            top: screenHeight * 0.04,
            left: screenWidth * 0.02,
            child: const UserCard(),
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