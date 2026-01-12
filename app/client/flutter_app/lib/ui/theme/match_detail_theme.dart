import 'package:flutter/material.dart';

class MatchDetailTheme {
  // Colors từ MatchDetailPage.css
  static const Color backgroundDark = Color(0xFF1A1A2E);
  static const Color cardBg = Color(0xFF1E1E2F); // Đổi sang màu solid hoặc gần như solid để text nổi bật
  static const Color roundTitleColor = Colors.white; // Màu cho Round
  static const Color yellowBadge = Color(0xFFFFDE59);
  static const Color errorRed = Color(0xFFE74C3C);
  static const Color textGrey = Color(0xFFB0BEC5); // Lighter grey for better contrast
  static const Color correctGreen = Color(0xFF2ECC71);
  static const Color wrongRed = Color(0xFFE74C3C);

  // Text Styles
  static const TextStyle roundBadgeStyle = TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 14, // Increased
    fontWeight: FontWeight.w800,
    color: Colors.black,
  );

  static const TextStyle questionTextStyle = TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 22, // Increased from 16
    fontWeight: FontWeight.w600,
    color: Colors.white,
  );

  static const TextStyle playerNameStyle = TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 18, // Increased from 14
    color: textGrey,
  );

  // Decoration cho Card câu hỏi
  static BoxDecoration questionCardDecoration = BoxDecoration(
    color: cardBg.withOpacity(0.95), // Tăng độ đậm để dễ đọc chữ trắng
    borderRadius: BorderRadius.circular(16),
    boxShadow: [
      BoxShadow(
        color: Colors.black.withOpacity(0.3),
        blurRadius: 10,
        offset: const Offset(0, 4),
      )
    ],
    border: const Border(
      left: BorderSide(color: yellowBadge, width: 6), // Làm vạch vàng dày hơn chút
    ),
  );

  // Nút Back Home đặc trưng
  static ButtonStyle backHomeButtonStyle = ElevatedButton.styleFrom(
    backgroundColor: errorRed,
    foregroundColor: Colors.white,
    padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
    elevation: 0,
  ).copyWith(
    // Hiệu ứng border-bottom của CSS
    side: WidgetStateProperty.all(const BorderSide(color: Color(0xFFC0392B), width: 4)),
  );
}
