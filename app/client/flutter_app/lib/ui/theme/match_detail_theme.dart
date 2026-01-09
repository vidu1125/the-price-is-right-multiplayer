import 'package:flutter/material.dart';

class MatchDetailTheme {
  // Colors từ MatchDetailPage.css
  static const Color backgroundDark = Color(0xFF1A1A2E);
  static const Color cardBg = Color(0x0DFFFFFF); // rgba(255, 255, 255, 0.05)
  static const Color yellowBadge = Color(0xFFFFDE59);
  static const Color errorRed = Color(0xFFE74C3C);
  static const Color textGrey = Color(0xFF8395A7);
  static const Color correctGreen = Color(0xFF2ECC71);
  static const Color wrongRed = Color(0xFFE74C3C);

  // Text Styles
  static const TextStyle roundBadgeStyle = TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 12,
    fontWeight: FontWeight.w800,
    color: Colors.black,
  );

  static const TextStyle questionTextStyle = TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 16,
    fontWeight: FontWeight.w600,
    color: Colors.white,
  );

  static const TextStyle playerNameStyle = TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 14,
    color: textGrey,
  );

  // Decoration cho Card câu hỏi
  static BoxDecoration questionCardDecoration = BoxDecoration(
    color: cardBg,
    borderRadius: BorderRadius.circular(16),
    border: const Border(
      left: BorderSide(color: yellowBadge, width: 5),
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
