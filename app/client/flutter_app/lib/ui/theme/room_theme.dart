import 'package:flutter/material.dart';

class RoomTheme {
  // Colors
  static const Color primaryDark = Color(0xFF1F2A44);
  static const Color accentYellow = Color(0xFFFFEB3B);
  static const Color accentGreen = Color(0xFF8BC34A);
  static const Color accentRed = Color(0xFFFF5252);
  static const Color panelBackground = Color(0xE61F2A44); // RGBA(31, 42, 68, 0.9)

  // Text Styles (Cần cài font Luckiest Guy trong pubspec.yaml)
  static const TextStyle titleStyle = TextStyle(
    fontFamily: 'LuckiestGuy',
    fontSize: 28,
    color: accentYellow,
    shadows: [Shadow(offset: Offset(3, 3), color: primaryDark)],
  );

  static const TextStyle cardNameStyle = TextStyle(
    fontFamily: 'LuckiestGuy',
    fontSize: 14,
    color: primaryDark,
  );

  static const TextStyle statusTagStyle = TextStyle(
    fontFamily: 'LuckiestGuy',
    fontSize: 13,
    color: Colors.white,
  );

  // Panel Decoration
  static BoxDecoration panelDecoration = BoxDecoration(
    color: panelBackground,
    borderRadius: BorderRadius.circular(18),
    border: Border.all(color: primaryDark, width: 4),
    boxShadow: [
      BoxShadow(
        color: Colors.black.withOpacity(0.4),
        blurRadius: 20,
        offset: const Offset(0, 8),
      ),
    ],
  );
}
