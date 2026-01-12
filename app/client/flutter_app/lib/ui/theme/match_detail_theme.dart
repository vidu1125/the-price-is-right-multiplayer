import 'package:flutter/material.dart';

class MatchDetailTheme {
  static const Color backgroundDark = Color(0xFF1A1A2E);
  // Thay đổi: Nền card sáng hơn, trong suốt hơn để tạo cảm giác nhẹ nhàng
  static const Color cardBg = Color(0xCCF8F9FA); 
  static const Color yellowBadge = Color(0xFFFFDE59);
  static const Color primaryBlue = Color(0xFF29B6F6);
  static const Color textDark = Color(0xFF2C3E50); // Chữ tối trên nền sáng
  static const Color errorRed = Color(0xFFE74C3C);
  static const Color correctGreen = Color(0xFF2ECC71);
  static const Color wrongRed = Color(0xFFE74C3C);
  static const Color textGrey = Color(0xFFB0BEC5); 

  static BoxDecoration questionCardDecoration = BoxDecoration(
    color: cardBg,
    borderRadius: BorderRadius.circular(24),
    border: Border.all(color: Colors.white.withOpacity(0.5), width: 1.5),
    boxShadow: [
      BoxShadow(
        color: Colors.black.withOpacity(0.1),
        blurRadius: 20,
        offset: const Offset(0, 10),
      ),
    ],
  );

  static const TextStyle questionTextStyle = TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 20,
    fontWeight: FontWeight.w700,
    color: textDark, 
  );

  static const TextStyle roundBadgeStyle = TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 20,
    fontWeight: FontWeight.w800,
    color: Colors.black,
  );

  static const TextStyle playerNameStyle = TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 22, 
    color: textDark, 
  );

  // Nút Back Home đặc trưng
  static ButtonStyle backHomeButtonStyle = ElevatedButton.styleFrom(
    backgroundColor: errorRed,
    foregroundColor: Colors.white,
    padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
    elevation: 0,
  ).copyWith(
    side: WidgetStateProperty.all(const BorderSide(color: Color(0xFFC0392B), width: 4)),
  );
}
