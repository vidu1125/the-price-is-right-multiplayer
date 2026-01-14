import 'package:flutter/material.dart';
import '../theme/game_theme.dart';

class BadgeWidget extends StatelessWidget {
  final String text;
  final Color color;
  final Color borderColor;

  const BadgeWidget({
    super.key, 
    required this.text, 
    required this.color,
    this.borderColor = GameTheme.darkBorder,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 18, vertical: 6),
      decoration: BoxDecoration(
        color: color,
        border: Border.all(color: borderColor, width: 3),
        borderRadius: BorderRadius.circular(12),
      ),
      child: Text(
        text, 
        style: const TextStyle(
          fontFamily: 'LuckiestGuy', 
          fontSize: 18, 
          color: Colors.black
        )
      ),
    );
  }
}
