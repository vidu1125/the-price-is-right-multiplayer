import 'package:flutter/material.dart';

class SettingsTheme {
  // Màu sắc đặc thù cho trang Settings
  static const Color headerGradientStart = Color(0xFF29B6F6);
  static const Color headerGradientEnd = Color(0xFF00ACC1);
  static const Color borderColor = Color(0xFFDCE3F0);
  static const Color textDark = Color(0xFF1F2A44);
  static const Color subText = Color(0xFF65748B);
  static const Color successGreen = Color(0xFF2B8A3E);
  static const Color errorRed = Color(0xFFC62828);

  // Cấu hình Card chính
  static BoxDecoration cardDecoration = BoxDecoration(
    color: Colors.white.withOpacity(0.96),
    border: Border.all(color: textDark, width: 2),
    borderRadius: BorderRadius.circular(20),
    boxShadow: const [
      BoxShadow(
        color: Colors.black12,
        blurRadius: 28,
        offset: Offset(0, 10),
      ),
    ],
  );

  // Style cho Header của Settings
  static BoxDecoration headerDecoration = const BoxDecoration(
    gradient: LinearGradient(
      colors: [headerGradientStart, headerGradientEnd],
      begin: Alignment.topLeft,
      end: Alignment.bottomRight,
    ),
    border: Border(
      bottom: BorderSide(color: textDark, width: 2),
    ),
  );

  // Style cho Input Field
  static InputDecoration inputDecoration(String hint) {
    return InputDecoration(
      hintText: hint,
      filled: true,
      fillColor: Colors.white,
      contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: const BorderSide(color: borderColor, width: 2),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: const BorderSide(color: headerGradientStart, width: 2),
      ),
    );
  }

  // Style cho nút chọn Avatar (Avatar Choice)
  static BoxDecoration avatarChoiceDecoration(bool isSelected) {
    return BoxDecoration(
      borderRadius: BorderRadius.circular(12),
      border: Border.all(
        color: isSelected ? headerGradientStart : borderColor,
        width: 2,
      ),
      boxShadow: isSelected 
        ? [BoxShadow(color: headerGradientStart.withOpacity(0.25), blurRadius: 0, spreadRadius: 3)] 
        : null,
    );
  }

  // Kiểu chữ tiêu đề phụ (Labels)
  static TextStyle labelStyle = const TextStyle(
    fontWeight: FontWeight.w600,
    color: textDark,
    fontSize: 14,
  );
}
