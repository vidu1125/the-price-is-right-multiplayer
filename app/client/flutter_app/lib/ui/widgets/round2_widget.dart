import 'package:flutter/material.dart';
import 'badge_widget.dart';
import 'game_button.dart';

class Round2Widget extends StatefulWidget {
  const Round2Widget({super.key});

  @override
  _Round2WidgetState createState() => _Round2WidgetState();
}

class _Round2WidgetState extends State<Round2Widget> {
  final TextEditingController _controller = TextEditingController();
  bool isWagerActive = false;

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            const BadgeWidget(text: "QUESTION 5/15", color: Color(0xFFFFDE00)),
            // Wager Button
            GestureDetector(
              onTap: () => setState(() => isWagerActive = !isWagerActive),
              child: Container(
                padding: const EdgeInsets.symmetric(horizontal: 15, vertical: 8),
                decoration: BoxDecoration(
                  color: isWagerActive ? const Color(0xFFF1C40F) : const Color(0xFFBDC3C7),
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(color: const Color(0xFF2D3436), width: 2),
                ),
                child: const Text("⭐ WAGER", style: TextStyle(fontFamily: 'LuckiestGuy')),
              ),
            )
          ],
        ),
        const Spacer(),
        // Placeholder for Question Text if needed, or just spacing
        const Text("Guess the price of this item!", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 32, color: Colors.white)),
        const Spacer(),
        Row(
          children: [
            Expanded(
              child: TextField(
                controller: _controller,
                decoration: InputDecoration(
                  fillColor: Colors.white, filled: true,
                  hintText: "Type your answer...",
                  border: OutlineInputBorder(borderRadius: BorderRadius.circular(15), borderSide: const BorderSide(width: 3)),
                ),
              ),
            ),
            const SizedBox(width: 15),
            GameButton(text: "SUBMIT", onPressed: () => print(_controller.text), color: const Color(0xFF2ECC71)),
          ],
        )
      ],
    );
  }
}
