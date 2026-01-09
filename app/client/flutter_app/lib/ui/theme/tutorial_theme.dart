import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class TutorialTheme {
  // Màu sắc chủ đạo từ file CSS
  static const Color primaryBlue = Color(0xFF29B6F6);
  static const Color darkBlue = Color(0xFF1F2A44);
  static const Color backgroundDark = Color(0xFF1A2332);
  static const Color accentBlue = Color(0xFF9BC2FF);

  // Hiệu ứng đổ bóng chữ đặc trưng của "The Price Is Right"
  static List<Shadow> gameShowShadow({double offset = 2.0}) {
    return [
      Shadow(offset: Offset(-offset, -offset), color: darkBlue),
      Shadow(offset: Offset(offset, -offset), color: darkBlue),
      Shadow(offset: Offset(-offset, offset), color: darkBlue),
      Shadow(offset: Offset(offset, offset), color: darkBlue),
      Shadow(offset: const Offset(0, 4), color: Colors.black.withOpacity(0.3), blurRadius: 8),
    ];
  }

  // Kiểu chữ tiêu đề (Luckiest Guy)
  static TextStyle headerStyle({double fontSize = 28}) {
    return GoogleFonts.luckiestGuy(
      fontSize: fontSize,
      color: primaryBlue,
      letterSpacing: 1,
      shadows: gameShowShadow(),
    );
  }

  // Kiểu chữ nội dung (Inter/Sans-serif)
  static TextStyle bodyStyle({bool isBold = false, double fontSize = 16}) {
    return TextStyle(
      fontFamily: 'Inter',
      fontSize: fontSize,
      color: backgroundDark,
      fontWeight: isBold ? FontWeight.bold : FontWeight.w500,
      height: 1.5,
    );
  }

  // Decoration cho các Card và Pill (Glassmorphism)
  static BoxDecoration glassDecoration() {
    return BoxDecoration(
      color: Colors.white.withOpacity(0.12),
      borderRadius: BorderRadius.circular(20),
      border: Border.all(color: Colors.white.withOpacity(0.2), width: 2),
      boxShadow: [
        BoxShadow(
          color: Colors.black.withOpacity(0.4),
          blurRadius: 60,
          offset: const Offset(0, 20),
        ),
      ],
    );
  }
}
