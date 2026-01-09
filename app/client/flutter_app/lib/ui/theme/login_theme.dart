import 'package:flutter/material.dart';

class LoginTheme {
  // --- COLORS ---
  static const Color primaryBlue = Color(0xFF0EA5E9);
  static const Color darkBlue = Color(0xFF2563EB);
  static const Color purple = Color(0xFF7C3AED);
  
  static const Color textDark = Color(0xFF0C1024);
  static const Color textSecondary = Color(0xFF4D5566);
  static const Color inputLabel = Color(0xFF20273A);
  static const Color inputBg = Color(0xFFF8FAFC);
  static const Color errorRed = Color(0xFFB3261E);

  // --- GRADIENTS ---
  static const LinearGradient backgroundGradient = LinearGradient(
    begin: Alignment.topLeft,
    end: Alignment.bottomRight,
    colors: [Color(0xFF0EA5E9), Color(0xFF7C3AED)],
  );

  static const LinearGradient ctaGradient = LinearGradient(
    begin: Alignment.topLeft,
    end: Alignment.bottomRight,
    colors: [primaryBlue, darkBlue, purple],
  );

  // --- TEXT STYLES ---
  static const TextStyle headingStyle = TextStyle(
    fontFamily: 'Poppins',
    fontSize: 28,
    fontWeight: FontWeight.bold,
    color: textDark,
  );

  static const TextStyle labelStyle = TextStyle(
    fontFamily: 'Poppins',
    fontSize: 14,
    fontWeight: FontWeight.w600,
    color: inputLabel,
  );

  // --- DECORATIONS ---
  static BoxDecoration cardDecoration = BoxDecoration(
    color: Colors.white,
    borderRadius: BorderRadius.circular(22),
    boxShadow: [
      BoxShadow(
        color: Colors.black.withOpacity(0.22),
        blurRadius: 70,
        offset: const Offset(0, 25),
      ),
    ],
  );

  static InputDecoration inputDecoration(String hint) {
    return InputDecoration(
      hintText: hint,
      hintStyle: const TextStyle(color: Colors.grey, fontSize: 14),
      filled: true,
      fillColor: inputBg,
      contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 18),
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: const BorderSide(color: Color(0xFFE2E8F0)),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: const BorderSide(color: Color(0xFFE2E8F0)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: const BorderSide(color: primaryBlue, width: 2),
      ),
    );
  }
}
