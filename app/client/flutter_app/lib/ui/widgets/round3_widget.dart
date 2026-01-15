import 'dart:math';
import 'package:flutter/material.dart';
import 'badge_widget.dart';
import 'game_button.dart';

class Round3Widget extends StatefulWidget {
  final List<String> segments;
  final int score;
  final int spinNumber;
  final bool showDecision;
  final VoidCallback onSpin;
  final Function(bool) onDecision;

  const Round3Widget({
    super.key, 
    required this.segments,
    required this.score,
    required this.spinNumber,
    required this.showDecision,
    required this.onSpin,
    required this.onDecision,
  });

  @override
  _Round3WidgetState createState() => _Round3WidgetState();
}

class _Round3WidgetState extends State<Round3Widget> with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _animation;
  double _rotationTurns = 0;
  bool _isSpinning = false;
  bool _dialogShown = false;
  int _lastSpunValue = 0;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(vsync: this, duration: const Duration(seconds: 4));
    _animation = CurvedAnimation(parent: _controller, curve: Curves.easeOutCubic);
    
    _controller.addStatusListener((status) {
      if (status == AnimationStatus.completed) {
        setState(() => _isSpinning = false);
        // If a decision is pending after animation, show the dialog
        if (widget.showDecision && !_dialogShown) {
          _showDecisionDialog();
        }
      }
    });
  }

  void _showDecisionDialog() {
    if (_dialogShown) return;
    _dialogShown = true;
    
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => AlertDialog(
        backgroundColor: const Color(0xFF1F2A44),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(24), side: const BorderSide(color: Color(0xFFFFDE00), width: 2)),
        title: const Text(
          "WHEEL RESULT", 
          textAlign: TextAlign.center,
          style: TextStyle(fontFamily: 'LuckiestGuy', color: Color(0xFFFFDE00), fontSize: 24),
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              "You spun: ${widget.score}",
              style: const TextStyle(color: Colors.white, fontSize: 28, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 10),
            const Text(
              "Would you like to keep this score or spin one last time?",
              textAlign: TextAlign.center,
              style: TextStyle(color: Colors.white70, fontSize: 16),
            ),
          ],
        ),
        actionsPadding: const EdgeInsets.all(20),
        actions: [
          Row(
            children: [
              Expanded(
                child: GameButton(
                  text: "TAKE SCORE", 
                  onPressed: () {
                    Navigator.pop(context);
                    widget.onDecision(false);
                    setState(() => _dialogShown = false);
                  }, 
                  color: Colors.orange
                ),
              ),
              const SizedBox(width: 10),
              Expanded(
                child: GameButton(
                  text: "SPIN AGAIN", 
                  onPressed: () {
                    Navigator.pop(context);
                    widget.onDecision(true);
                    setState(() {
                      _dialogShown = false;
                      _rotationTurns += 0.2; // Add a tiny nudge for visual feedback before next real result
                    });
                  }, 
                  color: const Color(0xFF2ECC71)
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  @override
  void didUpdateWidget(Round3Widget oldWidget) {
    super.didUpdateWidget(oldWidget);
    // If score changed and we aren't spinning, it means a new result arrived
    if (widget.score != oldWidget.score && widget.score != 0) {
      _startRotationAnimation(widget.score);
    }
    
    // Fallback: If decision becomes true but we aren't spinning (e.g. reconnected or skip animation)
    if (widget.showDecision && !oldWidget.showDecision && !_isSpinning && !_dialogShown) {
      WidgetsBinding.instance.addPostFrameCallback((_) => _showDecisionDialog());
    }
  }

  void _startRotationAnimation(int targetValue) {
    if (widget.segments.isEmpty) return;
    
    int index = widget.segments.indexOf(targetValue.toString());
    if (index == -1) return;

    // Logic to land on the top:
    // Wheel is drawn starting at 0 (Right). Top is at turns = 0.75 (270 deg).
    // Segment i center is at: (i + 0.5) / N turns.
    // To bring segment i to top: wheel must be rotated by: 0.75 - (i + 0.5)/N.
    
    double segmentTurn = (index + 0.5) / widget.segments.length;
    double targetTurn = 0.75 - segmentTurn;
    
    // Add multiple rotations for effect
    double extraRotations = 5.0; 
    
    setState(() {
      _isSpinning = true;
      _rotationTurns += extraRotations + (targetTurn - (_rotationTurns % 1.0));
    });
    
    _controller.forward(from: 0);
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  void _handleSpinClick() {
    if (_isSpinning || widget.showDecision) return;
    widget.onSpin();
    // Animation will start when we receive the result (score prop updates)
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(vertical: 20),
      child: Column(
        children: [
          // Header info
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              BadgeWidget(text: "TURN ${widget.spinNumber}/2", color: const Color(0xFFFFDE00)),
              BadgeWidget(text: "SCORE: ${widget.score}", color: const Color(0xFF2ECC71)),
            ],
          ),
          const SizedBox(height: 30),
          
          // Wheel with Pointer
          Expanded(
            child: Center(
              child: Stack(
                alignment: Alignment.center,
                children: [
                  // The Wheel
                  RotationTransition(
                    turns: _animation.drive(Tween(begin: _rotationTurns - 5, end: _rotationTurns)),
                    child: CustomPaint(
                      size: const Size(350, 350),
                      painter: WheelPainter(values: widget.segments),
                    ),
                  ),
                  
                  // Central hub
                  Container(
                    width: 40,
                    height: 40,
                    decoration: BoxDecoration(
                      color: Colors.white,
                      shape: BoxShape.circle,
                      boxShadow: [BoxShadow(color: Colors.black.withOpacity(0.5), blurRadius: 10)],
                    ),
                  ),
                  
                  // Top Pointer (Static)
                  Positioned(
                    top: 0,
                    child: CustomPaint(
                      size: const Size(30, 40),
                      painter: PointerPainter(),
                    ),
                  ),
                ],
              ),
            ),
          ),
          
          const SizedBox(height: 30),
          
          // Controls Area
          Container(
            height: 100,
            alignment: Alignment.center,
            child: widget.showDecision || _isSpinning
              ? Text(
                  _isSpinning ? "GOOD LUCK!" : "DECISION TIME...",
                  style: const TextStyle(color: Color(0xFFFFDE00), fontSize: 20, fontFamily: 'LuckiestGuy'),
                )
              : GameButton(
                  text: "SPIN THE WHEEL", 
                  onPressed: _handleSpinClick, 
                  color: const Color(0xFFE74C3C)
                ),
          ),
        ],
      ),
    );
  }
}

class PointerPainter extends CustomPainter {
  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = Colors.red
      ..style = PaintingStyle.fill;
    
    final path = Path();
    path.moveTo(0, 0);
    path.lineTo(size.width, 0);
    path.lineTo(size.width / 2, size.height);
    path.close();
    
    // Add border
    final borderPaint = Paint()
      ..color = Colors.white
      ..style = PaintingStyle.stroke
      ..strokeWidth = 2;

    canvas.drawPath(path, paint);
    canvas.drawPath(path, borderPaint);
  }

  @override
  bool shouldRepaint(CustomPainter oldDelegate) => false;
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
      // Use more vibrant alternating colors
      paint.color = i % 2 == 0 ? const Color(0xFF2980B9) : const Color(0xFF3498DB);
      canvas.drawArc(Rect.fromCircle(center: center, radius: radius), i * angle, angle, true, paint);
      
      // Draw segment border
      final borderPaint = Paint()
        ..color = Colors.white.withOpacity(0.3)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1;
      canvas.drawArc(Rect.fromCircle(center: center, radius: radius), i * angle, angle, true, borderPaint);

      // Draw Text
      final textPainter = TextPainter(
        text: TextSpan(
          text: values[i],
          style: const TextStyle(color: Colors.white, fontSize: 20, fontWeight: FontWeight.bold),
        ),
        textDirection: TextDirection.ltr,
      );
      textPainter.layout();

      final textAngle = i * angle + angle / 2;
      final textRadius = radius * 0.75; 
      final x = center.dx + textRadius * cos(textAngle) - textPainter.width / 2;
      final y = center.dy + textRadius * sin(textAngle) - textPainter.height / 2;
      
      canvas.save();
      canvas.translate(x + textPainter.width/2, y + textPainter.height/2);
      canvas.rotate(textAngle + pi/2); 
      canvas.translate(-(x + textPainter.width/2), -(y + textPainter.height/2));
      textPainter.paint(canvas, Offset(x, y));
      canvas.restore();
    }

    // Outer wheel border
    final outerBorder = Paint()
      ..color = const Color(0xFFF1C40F)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 8;
    canvas.drawCircle(center, radius, outerBorder);
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
