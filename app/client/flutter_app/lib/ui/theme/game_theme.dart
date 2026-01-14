import 'package:flutter/material.dart';

class GameTheme {
  static const Color primaryYellow = Color(0xFFFFDE00);
  static const Color accentRed = Color(0xFFFF5E5E);
  static const Color darkBorder = Color(0xFF2D3436);
  static const Color backgroundOverlay = Color(0x33000000);

  static TextStyle get gameTextStyle => const TextStyle(
    fontFamily: 'LuckiestGuy',
    color: Colors.white,
    shadows: [
      Shadow(offset: Offset(-1.5, -1.5), color: Color(0xFF2D3436)),
      Shadow(offset: Offset(1.5, -1.5), color: Color(0xFF2D3436)),
      Shadow(offset: Offset(-1.5, 1.5), color: Color(0xFF2D3436)),
      Shadow(offset: Offset(1.5, 1.5), color: Color(0xFF2D3436)),
    ],
  );
}
