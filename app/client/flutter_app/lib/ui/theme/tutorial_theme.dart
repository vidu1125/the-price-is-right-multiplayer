import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class TutorialTheme {
  static const Color primaryBlue = Color(0xFF29B6F6);
  static const Color darkBlue = Color(0xFF1F2A44); // Restored
  static const Color brightYellow = Color(0xFFFFDE59);
  static const Color glassWhite = Color(0x33FFFFFF); // Trắng trong suốt
  static const Color contentBg = Color(0xFFF0F7FF); // Màu nền nội dung sáng

  static BoxDecoration glassDecoration({double opacity = 0.15}) {
    return BoxDecoration(
      color: Colors.white.withOpacity(opacity),
      borderRadius: BorderRadius.circular(24),
      border: Border.all(color: Colors.white.withOpacity(0.3), width: 1.5),
      boxShadow: [
        BoxShadow(
          color: Colors.black.withOpacity(0.1),
          blurRadius: 20,
          offset: const Offset(0, 10),
        ),
      ],
    );
  }

  static TextStyle gameFont({double size = 24, Color color = Colors.white}) {
    return GoogleFonts.luckiestGuy(fontSize: size, color: color, letterSpacing: 1.2);
  }

  static List<Shadow> gameShowShadow({double offset = 2.0}) {
    return [
      Shadow(offset: Offset(-offset, -offset), color: const Color(0xFF1F2A44)),
      Shadow(offset: Offset(offset, -offset), color: const Color(0xFF1F2A44)),
      Shadow(offset: Offset(-offset, offset), color: const Color(0xFF1F2A44)),
      Shadow(offset: Offset(offset, offset), color: const Color(0xFF1F2A44)),
      Shadow(offset: const Offset(0, 4), color: Colors.black.withOpacity(0.3), blurRadius: 8),
    ];
  }

  static BoxDecoration contentBoxDecoration = BoxDecoration(
    color: Colors.white.withOpacity(0.9), // Nền trắng sữa chiếm 90% để làm nổi bật chữ đen/xanh
    borderRadius: BorderRadius.circular(24),
    border: Border.all(color: Colors.white, width: 2),
    boxShadow: [
      BoxShadow(
        color: Colors.black.withOpacity(0.1),
        blurRadius: 20,
        offset: const Offset(0, 10),
      ),
    ],
  );

  static TextStyle contentTitleStyle = GoogleFonts.luckiestGuy(
    fontSize: 32,
    color: const Color(0xFF0D47A1), // Màu xanh đậm để tương phản tốt trên nền trắng
    letterSpacing: 1,
  );
  static BoxDecoration whiteContentCard = BoxDecoration(
    color: Colors.white.withOpacity(0.95), // Trắng gần như tuyệt đối để chữ rõ nét nhất
    borderRadius: BorderRadius.circular(30),
    boxShadow: [
      BoxShadow(
        color: Colors.black.withOpacity(0.2),
        blurRadius: 15,
        offset: const Offset(0, 8),
      ),
      // Hiệu ứng viền sáng phía trên để tạo khối 3D nhẹ
      const BoxShadow(
        color: Colors.white,
        blurRadius: 1,
        offset: Offset(-2, -2),
      ),
    ],
  );

  static TextStyle detailTextStyle = const TextStyle(
    fontFamily: 'Parkinsans',
    fontSize: 28,
    color: Color(0xFF2C3E50), // Màu xanh xám đậm, sang trọng hơn màu đen thuần
    height: 1.4,
  );
}
