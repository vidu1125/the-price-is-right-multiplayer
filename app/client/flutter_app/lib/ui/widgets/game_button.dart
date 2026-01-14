import 'package:flutter/material.dart';
import '../theme/game_theme.dart';

class GameButton extends StatelessWidget {
  final String text;
  final VoidCallback onPressed;
  final Color color;

  const GameButton({
    super.key,
    required this.text,
    required this.onPressed,
    this.color = GameTheme.accentRed
  });

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: onPressed,
        borderRadius: BorderRadius.circular(15),
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 10),
          decoration: BoxDecoration(
            color: color,
            borderRadius: BorderRadius.circular(15),
            border: Border.all(color: GameTheme.darkBorder, width: 3),
            boxShadow: [
              BoxShadow(
                color: Colors.black.withOpacity(0.3),
                offset: const Offset(0, 4),
                blurRadius: 0,
              )
            ],
          ),
          alignment: Alignment.center,
          child: Text(text, style: GameTheme.gameTextStyle.copyWith(fontSize: 28)),
        ),
      ),
    );
  }
}
