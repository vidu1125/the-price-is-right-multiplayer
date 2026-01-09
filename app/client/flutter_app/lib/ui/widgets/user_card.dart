import 'package:flutter/material.dart';
import '../theme/lobby_theme.dart';

class UserCard extends StatelessWidget {
  const UserCard({super.key});

  @override
  Widget build(BuildContext context) {
    return Container(
      // 1. Box hình viên thuốc (Pill shape) từ CSS .user-card
      decoration: LobbyTheme.userCardDecoration,
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          // 2. Avatar với viền kép (Double border effect)
          GestureDetector(
            onTap: () => print("Open profile modal"),
            child: Container(
              width: 48,
              height: 48,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: const Color(0xFFF8F8F8), // Nền trắng nhẹ cho avatar
                border: Border.all(color: LobbyTheme.primaryDark, width: 4),
              ),
              child: ClipOval(
                child: Image.asset(
                  'assets/images/default-mushroom.jpg',
                  fit: BoxFit.cover,
                  // Dự phòng nếu không tìm thấy ảnh
                  errorBuilder: (context, error, stackTrace) => const Icon(
                    Icons.person,
                    color: LobbyTheme.primaryDark,
                    size: 30,
                  ),
                ),
              ),
            ),
          ),
          
          const SizedBox(width: 12),
          
          // 3. Player Name với Shadow đặc trưng từ CSS
          Text(
            "Player Name",
            style: TextStyle(
              fontFamily: 'LuckiestGuy',
              fontSize: 18,
              color: LobbyTheme.primaryDark,
              // Hiệu ứng text-shadow: 1px 1px 0 #fff trong CSS
              shadows: const [
                Shadow(offset: Offset(1, 1), color: Colors.white),
                Shadow(offset: Offset(-1, -1), color: Colors.white),
              ],
            ),
          ),
        ],
      ),
    );
  }
}