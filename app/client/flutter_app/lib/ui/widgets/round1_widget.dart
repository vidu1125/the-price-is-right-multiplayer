import 'package:flutter/material.dart';
import '../theme/game_theme.dart';

class Round1Widget extends StatefulWidget {
  final Map questionData;
  final Function(int) onAnswerSelected;

  const Round1Widget({super.key, required this.questionData, required this.onAnswerSelected});

  @override
  // ignore: library_private_types_in_public_api
  _Round1WidgetState createState() => _Round1WidgetState();
}

class _Round1WidgetState extends State<Round1Widget> {
  int? selectedIndex;
  bool isAnswered = false;

  @override
  void didUpdateWidget(Round1Widget oldWidget) {
    super.didUpdateWidget(oldWidget);
    // Reset answer state when new question is received
    if (oldWidget.questionData['question_idx'] != widget.questionData['question_idx']) {
      setState(() {
        selectedIndex = null;
        isAnswered = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    List choices = widget.questionData['choices'] ?? [];
    int? correctIndex = widget.questionData['correctIndex'];

    return Column(
      children: [
        // Question Indicator
        Align(
          alignment: Alignment.centerLeft,
          child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 18, vertical: 6),
            decoration: BoxDecoration(
              color: const Color(0xFFFFDE00),
              border: Border.all(color: const Color(0xFF2D3436), width: 3),
              borderRadius: BorderRadius.circular(12),
            ),
            child: Text(
              "QUESTION ${(widget.questionData['question_idx'] ?? 0) + 1}/${widget.questionData['total_questions'] ?? 10}",
              style: const TextStyle(fontFamily: 'LuckiestGuy', fontSize: 24)
            ),
          ),
        ),
        const SizedBox(height: 50),
        
        // Question Text
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 20),
          child: Text(
            widget.questionData['question'], 
            style: GameTheme.gameTextStyle.copyWith(fontSize: 48), 
            textAlign: TextAlign.center
          ),
        ),
        
        const Spacer(),

        // Answers Grid
        GridView.builder(
          shrinkWrap: true,
          physics: const NeverScrollableScrollPhysics(),
          gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
            crossAxisCount: 2, 
            crossAxisSpacing: 30, 
            mainAxisSpacing: 30, 
            childAspectRatio: 3.5,
          ),
          itemCount: choices.length,
          itemBuilder: (context, index) {
            Color btnColor = Colors.white;
            if (isAnswered) {
              if (correctIndex != null) {
                 // Result received
                 if (index == correctIndex) btnColor = const Color(0xFF2ECC71);
                 else if (index == selectedIndex) btnColor = const Color(0xFFE74C3C);
                 else btnColor = Colors.white.withOpacity(0.3);
              } else {
                 // Waiting for result
                 if (index == selectedIndex) btnColor = const Color(0xFF3498DB);
                 else btnColor = Colors.white.withOpacity(0.3);
              }
            }

            return GestureDetector(
              onTap: isAnswered ? null : () {
                setState(() {
                  selectedIndex = index;
                  isAnswered = true;
                });
                widget.onAnswerSelected(index);
              },
              child: AnimatedContainer(
                duration: const Duration(milliseconds: 200),
                decoration: BoxDecoration(
                  color: btnColor,
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(color: const Color(0xFF2D3436), width: 5),
                  boxShadow: [
                    BoxShadow(
                      offset: const Offset(0, 6), 
                      color: const Color(0xFF2D3436).withOpacity(0.5)
                    )
                  ],
                ),
                alignment: Alignment.center,
                child: Text(
                  choices[index].toString(), 
                  style: TextStyle(
                    fontFamily: 'LuckiestGuy',
                    color: (isAnswered && index != correctIndex && index != selectedIndex) 
                        ? Colors.white70 
                        : (isAnswered ? Colors.white : Colors.black87), 
                    fontSize: 28, 
                    fontWeight: FontWeight.bold
                  )
                ),
              ),
            );
          },
        ),
        const SizedBox(height: 50),
      ],
    );
  }
}
