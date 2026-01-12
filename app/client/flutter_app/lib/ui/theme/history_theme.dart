import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class HistoryTheme {
  static const Color primaryBlue = Color(0xFF29B6F6);
  static const Color primaryGold = Color(0xFFFFD700); // Restored
  static const Color successGreen = Color(0xFF4CAF50); // Restored
  static const Color darkBlue = Color(0xFF1F2A44);
  static const Color glassWhite = Color(0x1EFFFFFF); // white with 0.12 opacity
  static const Color forfeitRed = Color(0xFFEF5350);
  static const Color eliminatedOrange = Color(0xFFFF7043);

  static TextStyle statusTextStyle(Color color) => GoogleFonts.luckiestGuy(
    fontSize: 12,
    color: color,
    letterSpacing: 0.5,
  );

  // Tái tạo text-shadow từ TutorialPage.css
  static List<Shadow> gameShowShadow = const [
    Shadow(offset: Offset(-2, -2), color: darkBlue),
    Shadow(offset: Offset(2, -2), color: darkBlue),
    Shadow(offset: Offset(-2, 2), color: darkBlue),
    Shadow(offset: Offset(2, 2), color: darkBlue),
    Shadow(offset: Offset(0, 4), color: Colors.black38, blurRadius: 8),
  ];
  static List<Shadow> textOutlineShadow = gameShowShadow; // Alias

  static TextStyle statValueStyle = GoogleFonts.luckiestGuy(
    letterSpacing: 1,
    shadows: const [Shadow(offset: Offset(0, 2), blurRadius: 2)],
  );

  static BoxDecoration statsPanelDecoration = BoxDecoration(
    color: Colors.white.withOpacity(0.05),
    borderRadius: BorderRadius.circular(16),
    border: Border.all(color: primaryBlue.withOpacity(0.5), width: 2),
  );

  static TextStyle titleStyle = GoogleFonts.luckiestGuy(
    fontSize: _clampDouble(28, 42), // phỏng theo clamp trong CSS
    color: primaryBlue,
    letterSpacing: 1,
    shadows: gameShowShadow,
  );

  static TextStyle columnHeaderStyle = GoogleFonts.luckiestGuy(
    fontSize: 14,
    color: const Color(0xFF9BC2FF),
    letterSpacing: 1,
  );

  static BoxDecoration glassCardDecoration = BoxDecoration(
    color: const Color(0xF21A2332).withOpacity(0.8), // Dark navy glass
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

  // Hàm hỗ trợ clamp cho Flutter cũ
  static double _clampDouble(double min, double max) => min;
}
