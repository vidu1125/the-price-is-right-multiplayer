import 'dart:math';
import 'package:flutter/material.dart';
import 'badge_widget.dart';
import 'game_button.dart';

class Round3Widget extends StatefulWidget {
  const Round3Widget({super.key});

  @override
  _Round3WidgetState createState() => _Round3WidgetState();
}

class _Round3WidgetState extends State<Round3Widget> with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _animation;
  double _rotation = 0;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(vsync: this, duration: const Duration(seconds: 4));
    _animation = CurvedAnimation(parent: _controller, curve: Curves.easeOutCubic);
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  void _spin() {
    if (_controller.isAnimating) return;
    double randomRotation = (Random().nextDouble() * 360) + 1800; // Quay ít nhất 5 vòng
    setState(() {
      _rotation += randomRotation;
    });
    _controller.forward(from: 0);
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        const BadgeWidget(text: "TURN 1/2", color: Color(0xFFFFDE00)),
        const SizedBox(height: 40),
        Expanded(
          child: Center(
            child: RotationTransition(
              turns: _animation.drive(Tween(begin: 0, end: _rotation / 360)),
              child: CustomPaint(
                size: const Size(300, 300),
                painter: WheelPainter(values: ["100", "200", "500", "0", "1000", "LOSE"]),
              ),
            ),
          ),
        ),
        const SizedBox(height: 40),
        GameButton(text: "SPIN", onPressed: _spin, color: const Color(0xFFE74C3C)),
        const SizedBox(height: 40),
      ],
    );
  }
}

class WheelPainter extends CustomPainter {
  final List<String> values;
  WheelPainter({required this.values});

  @override
  void paint(Canvas canvas, Size size) {
    final double angle = 2 * pi / values.length;
    final paint = Paint()..style = PaintingStyle.fill;
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2;

    for (int i = 0; i < values.length; i++) {
      paint.color = i % 2 == 0 ? Colors.blueAccent : Colors.lightBlue;
      canvas.drawArc(Rect.fromCircle(center: center, radius: radius), i * angle, angle, true, paint);
      
      // Draw Text
      final textPainter = TextPainter(
        text: TextSpan(
          text: values[i],
          style: const TextStyle(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold),
        ),
        textDirection: TextDirection.ltr,
      );
      textPainter.layout();

      // Position text
      final textAngle = i * angle + angle / 2;
      final textRadius = radius * 0.7; // Position text at 70% of radius
      final x = center.dx + textRadius * cos(textAngle) - textPainter.width / 2;
      final y = center.dy + textRadius * sin(textAngle) - textPainter.height / 2;

      // Rotate canvas for text? Or just place it. 
      // Ideally rotate context to align text with slice, but simple x,y is okay for now.
      // Let's do simple placement.
      
      canvas.save();
      canvas.translate(x + textPainter.width/2, y + textPainter.height/2);
      canvas.rotate(textAngle + pi/2); // Rotate text to point outward/inward
      canvas.translate(-(x + textPainter.width/2), -(y + textPainter.height/2));
      textPainter.paint(canvas, Offset(x, y));
      canvas.restore();
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
