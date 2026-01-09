import 'package:flutter/material.dart';

class LobbyTheme {
  // --- COLORS ---
  static const Color primaryDark = Color(0xFF1F2A44); // Màu xanh đậm viền
  static const Color backgroundNavy = Color(0xFF1A1A2E); // Màu nền lobby
  static const Color yellowGame = Color(0xFFFFEB3B); // Màu vàng rực rỡ
  static const Color blueAction = Color(0xFF29B6F6); // Màu xanh nút side-actions
  static const Color redLogout = Color(0xFFFF7043); // Màu đỏ nút logout
  static const Color greenCreate = Color(0xFF8BC34A); // Màu xanh lá nút create
  static const Color yellowReload = Color(0xFFFFD54F); // Màu vàng nút reload
  static const Color rowBackground = Color(0xB3465A78); // rgba(70, 90, 120, 0.7)

  // Helper tạo Text Shadow giống Web (viền đen xung quanh chữ)
  static List<Shadow> gameTextOutline = const [
    Shadow(offset: Offset(-1.5, -1.5), color: primaryDark),
    Shadow(offset: Offset(1.5, -1.5), color: primaryDark),
    Shadow(offset: Offset(-1.5, 1.5), color: primaryDark),
    Shadow(offset: Offset(1.5, 1.5), color: primaryDark),
  ];

  // --- TEXT STYLES ---
  
  // Font chính cho Tiêu đề và Nút (Luckiest Guy)
  static TextStyle gameFont({
    double fontSize = 18,
    Color color = Colors.white,
  }) {
    return TextStyle(
      fontFamily: 'LuckiestGuy', // Đảm bảo đã khai báo trong pubspec.yaml
      fontSize: fontSize,
      color: color,
      letterSpacing: 1.2,
      shadows: gameTextOutline, // Áp dụng viền chữ
    );
  }

  // Font cho nội dung bảng (Parkinsans)
  static const TextStyle tableContentFont = TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 12,
    fontWeight: FontWeight.w400,
    color: Colors.white,
  );

  // --- DECORATIONS ---

  // Style cho Room Panel (Cột phải)
  static BoxDecoration roomPanelDecoration = BoxDecoration(
    color: const Color(0xB31F2A44), // rgba(31, 42, 68, 0.7)
    borderRadius: BorderRadius.circular(18),
    border: Border.all(color: primaryDark, width: 4),
    boxShadow: [
      BoxShadow(
        color: Colors.black.withOpacity(0.5),
        blurRadius: 30,
        offset: const Offset(0, 10),
      ),
    ],
  );

  // Style cho User Card (Viên thuốc màu vàng)
  static BoxDecoration userCardDecoration = BoxDecoration(
    color: yellowGame, // #FFEB3B
    borderRadius: BorderRadius.circular(30),
    border: Border.all(color: primaryDark, width: 4), // Viền 4px chuẩn CSS
    boxShadow: [
      BoxShadow(
        color: Colors.black.withOpacity(0.3),
        offset: const Offset(0, 6),
        blurRadius: 16,
      ),
    ],
  );

  // Style cho các hàng trong bảng
  static BoxDecoration tableRowDecoration = BoxDecoration(
    color: rowBackground,
    borderRadius: BorderRadius.circular(12),
    border: Border.all(color: primaryDark, width: 2),
  );

  // --- BUTTON STYLES ---

  // Button Style chung cho Side Actions (Bên trái)
  static ButtonStyle sideButtonStyle = ElevatedButton.styleFrom(
    backgroundColor: blueAction,
    foregroundColor: Colors.white,
    elevation: 8,
    shadowColor: Colors.black.withOpacity(0.5),
    padding: const EdgeInsets.symmetric(vertical: 20),
    shape: RoundedRectangleBorder(
      borderRadius: BorderRadius.circular(14),
      side: const BorderSide(color: primaryDark, width: 2.5),
    ),
  ).copyWith(
    // Hiệu ứng nhấn (nút lún xuống một chút giống CSS :active)
    overlayColor: WidgetStateProperty.all(Colors.white10),
  );

  static ButtonStyle logoutButtonStyle = sideButtonStyle.copyWith(
    backgroundColor: WidgetStateProperty.all(Color(0xFFFF7043)),
    side: WidgetStateProperty.all(const BorderSide(color: Color(0xFF7A1F1F), width: 2.5)),
  );
}