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
    return Scaffold(
      backgroundColor: LobbyTheme.backgroundNavy,
      body: Stack(
        children: [
          // Background Image
          Positioned.fill(
            child: Image.asset("assets/images/lobby-bg.png", fit: BoxFit.cover),
          ),
          Positioned.fill(child: Container(color: Colors.black.withOpacity(0.5))),

          // 1. User Card (Góc trên bên trái - Không ảnh hưởng đến các nút bên dưới)
          const Positioned(
            top: 50,
            left: 30,
            child: UserCard(),
          ),

          SafeArea(
            child: Column(
              children: [
                const AppTitle(), // Tiêu đề căn giữa
                Expanded(
                  child: Padding(
                    padding: const EdgeInsets.fromLTRB(40, 20, 40, 40),
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.center, // Căn giữa theo chiều dọc
                      children: [
                        // Cột bên trái: Các nút hành động
                        SizedBox(
                          width: 220,
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.center, // Các nút gom lại giữa cột
                            children: [
                              _sideButton("VIEW HISTORY", () {
                                Navigator.pushNamed(context, '/history');
                              }),
                              _sideButton("VIEW TUTORIAL", () {
                                Navigator.pushNamed(context, '/tutorial');
                              }),
                              _sideButton("SETTING", () {
                                Navigator.pushNamed(context, '/settings');
                              }),
                              const SizedBox(height: 40), // Khoảng cách riêng cho nút Logout
                              _sideButton("LOG OUT", () async {
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
                        
                        const SizedBox(width: 40),

                        // Cột bên phải: Room Panel
                        const Expanded(
                          child: RoomCard(),
                        ),
                      ],
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

  Widget _sideButton(String label, VoidCallback onPressed, {bool isLogout = false}) {
    return Container(
      margin: const EdgeInsets.only(bottom: 20),
      width: double.infinity,
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(14),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.25),
            offset: const Offset(0, 6), // Đổ bóng xuống dưới
            blurRadius: 16,
          )
        ],
      ),
      child: ElevatedButton(
        style: isLogout ? LobbyTheme.logoutButtonStyle : LobbyTheme.sideButtonStyle,
        onPressed: onPressed,
        child: Text(
          label,
          style: LobbyTheme.gameFont(fontSize: 20), // Tăng size font cho giống Web
        ),
      ),
    );
  }
}