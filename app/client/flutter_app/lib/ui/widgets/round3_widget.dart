import 'dart:math';
import 'package:flutter/material.dart';
import 'badge_widget.dart';
import 'game_button.dart';
import 'package:google_fonts/google_fonts.dart';

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

  // Segments matching backend logic (10-100 in steps of 10)
  // Scrambled order for better gameplay feel (10 items)
  final List<String> _hardcodedSegments = [
    "100", "50", "90", "20", "80", "60", "30", "70", "40", "10"
  ];
  
  // Base colors - we'll cycle through these if exact key missing
  final List<Color> _palette = [
    const Color(0xFFFFCC33),   // Yellow
    const Color(0xFFE67E22),   // Orange
    const Color(0xFFE74C3C),   // Red
    const Color(0xFF9B59B6),   // Purple
    const Color(0xFF3498DB),   // Blue
    const Color(0xFF2ECC71),   // Green
  ];
  
  Color _getSegmentColor(String label, int index) {
     if (label == "100") return const Color(0xFFFFD700); // Gold for 100
     return _palette[index % _palette.length];
  }

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(vsync: this, duration: const Duration(seconds: 4));
    _animation = CurvedAnimation(parent: _controller, curve: Curves.easeOutCubic);
    
    _controller.addStatusListener((status) {
      if (status == AnimationStatus.completed) {
        setState(() => _isSpinning = false);
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
        title: Text(
          "WHEEL RESULT", 
          textAlign: TextAlign.center,
          style: GoogleFonts.luckiestGuy(color: const Color(0xFFFFDE00), fontSize: 24),
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              "You spun: ${widget.score == -1 ? 'LOSE' : widget.score}",
              style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 32),
            ),
            const SizedBox(height: 10),
            Text(
              "Would you like to keep this score or spin again?",
              textAlign: TextAlign.center,
              style: GoogleFonts.outfit(color: Colors.white70, fontSize: 16),
            ),
          ],
        ),
        actionsPadding: const EdgeInsets.all(20),
        actions: [
          Row(
            children: [
              Expanded(
                child: GameButton(
                  text: "TAKE", 
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
                  text: "AGAIN", 
                  onPressed: () {
                    Navigator.pop(context);
                    widget.onDecision(true);
                    setState(() => _dialogShown = false);
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

  int _lastProcessedSpin = 0;
  bool _spinRequested = false;

  @override
  void didUpdateWidget(Round3Widget oldWidget) {
    super.didUpdateWidget(oldWidget);
    
    // Ignore pending/invalid scores entirely
    if (widget.score == -999) return;
    
    // Check if we have a new spin result to process
    // Logic: score changed from previous state, OR spinNumber incresed but score is fresh
    if (widget.score != -1 && widget.score != -999 && widget.score != 0) {
        // If we were waiting for result (spin requested), animate now
        if (_spinRequested) {
             print("[Round3] Starting animation to target: ${widget.score}");
            _startRotationAnimation(widget.score);
            _spinRequested = false; // Reset request flag
        } else if (!_isSpinning && _rotationTurns == 0) {
            // Re-joining or initial load with existing score - just snap or animate
             _startRotationAnimation(widget.score);
        }
    }
    
    // Reset for 2nd spin if needed
    if (widget.spinNumber > _lastProcessedSpin) {
        if (widget.spinNumber == 2) {
             print("[Round3] Resetting for Spin 2");
            // Reset UI for second spin if needed, though usually score update handles it
            // _dialogShown = false; // Handled in action
        }
        _lastProcessedSpin = widget.spinNumber;
    }

    if (widget.showDecision && !oldWidget.showDecision && !_isSpinning && !_dialogShown) {
      WidgetsBinding.instance.addPostFrameCallback((_) => _showDecisionDialog());
    }
  }

  void _startRotationAnimation(int targetValue) {
    String searchStr = targetValue == -1 ? "LOSE" : targetValue.toString();
    int index = _hardcodedSegments.indexOf(searchStr);
    if (index == -1) return;

    int n = _hardcodedSegments.length;
    double segmentTurn = (index + 0.5) / n;
    
    // Pointer is at TOP (0.25 turns offset from the painter's -pi/2 start).
    // Segment i center is at -0.25 + segmentTurn.
    // To land at TOP (0 turns absolute):
    // Rotation = 0 - (-0.25 + segmentTurn) = 0.25 - segmentTurn.
    
    double destinationTurn = 0.25 - segmentTurn;
    double extraRotations = 6.0;
    
    double currentNormalized = _rotationTurns % 1.0;
    double neededTurn = destinationTurn - currentNormalized;
    while (neededTurn <= 0) neededTurn += 1.0; 
    
    setState(() {
      _isSpinning = true;
      _rotationTurns += extraRotations + neededTurn;
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
    setState(() {
      _spinRequested = true;
    });
    print("[Round3] Spin requested by player click");
    widget.onSpin();
  }

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final double wheelSize = min(constraints.maxWidth, constraints.maxHeight) * 0.75;
        
        return Stack(
          alignment: Alignment.center,
          children: [
            // Top Row Information
            Positioned(
              top: 20,
              left: 20,
              child: BadgeWidget(text: "SPIN ${widget.spinNumber}/2", color: const Color(0xFFFFDE00)),
            ),
            Positioned(
              top: 60,
              left: 20,
              child: Text(
                "TAP THE RED BUTTON TO SPIN!", 
                style: GoogleFonts.luckiestGuy(color: Colors.white70, fontSize: 14),
              ),
            ),
            Positioned(
              top: 20,
              right: 20,
              child: Container(
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
                decoration: BoxDecoration(
                  color: const Color(0xFF8E44AD).withOpacity(0.9),
                  borderRadius: BorderRadius.circular(16),
                  border: Border.all(color: Colors.white, width: 3),
                  boxShadow: [
                    BoxShadow(color: Colors.black.withOpacity(0.4), blurRadius: 10, offset: const Offset(0, 4)),
                  ],
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    const Icon(Icons.stars, color: Colors.yellow, size: 24),
                    const SizedBox(width: 8),
                    Text(
                      "ROUND SCORE: ${widget.score == -1 || widget.score == -999 ? 0 : widget.score}",
                      style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 18),
                    ),
                  ],
                ),
              ),
            ),

            // Main Wheel Area
            Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  // POINTER (The requested Indicator shape)
                  Container(
                    width: 60,
                    height: 60,
                    margin: const EdgeInsets.only(bottom: 10),
                    child: CustomPaint(
                      painter: PointerPainter(),
                    ),
                  ),
                  
                  const SizedBox(height: 5),
                  
                  Stack(
                    alignment: Alignment.center,
                    children: [
                      // Decorative Frame
                      Container(
                        width: wheelSize + 40,
                        height: wheelSize + 40,
                        decoration: BoxDecoration(
                          shape: BoxShape.circle,
                          color: const Color(0xFF2D3436),
                          border: Border.all(color: const Color(0xFFFFDE00), width: 10),
                          boxShadow: [
                            BoxShadow(color: Colors.black.withOpacity(0.6), blurRadius: 20, offset: const Offset(0, 15)),
                          ],
                        ),
                      ),
                      
                      // Rotating Wheel
                      RotationTransition(
                        turns: _animation.drive(Tween(
                          begin: _rotationTurns > 0 ? _rotationTurns - 6 : 0, 
                          end: _rotationTurns
                        )),
                        child: CustomPaint(
                          size: Size(wheelSize, wheelSize),
                          painter: WheelPainter(
                            labels: _hardcodedSegments,
                            colorProvider: _getSegmentColor,
                          ),
                        ),
                      ),
                      
                      // Center Hub
                      Container(
                        width: 60,
                        height: 60,
                        decoration: BoxDecoration(
                          color: const Color(0xFF34495E),
                          shape: BoxShape.circle,
                          border: Border.all(color: Colors.white, width: 5),
                          boxShadow: [BoxShadow(color: Colors.black.withOpacity(0.5), blurRadius: 8)],
                        ),
                        child: Center(
                          child: Icon(Icons.bolt, color: Colors.yellow.withOpacity(0.8), size: 30),
                        ),
                      ),
                    ],
                  ),
                ],
              ),
            ),

            // Large Spin Button on the side
            Positioned(
              right: constraints.maxWidth * 0.05,
              child: _buildSpinButton(),
            ),
          ],
        );
      },
    );
  }

  Widget _buildSpinButton() {
    bool canSpin = !widget.showDecision && !_isSpinning;
    String btnText = widget.spinNumber == 1 ? "1ST" : "2ND";

    return GestureDetector(
      onTap: _handleSpinClick,
      child: AnimatedScale(
        scale: canSpin ? 1.0 : 0.85,
        duration: const Duration(milliseconds: 300),
        curve: Curves.elasticOut,
        child: Container(
          width: 140,
          height: 140,
          decoration: BoxDecoration(
            color: canSpin ? const Color(0xFFE74C3C) : Colors.blueGrey.shade700,
            shape: BoxShape.circle,
            border: Border.all(color: Colors.white, width: 8),
            boxShadow: [
              BoxShadow(
                color: Colors.black.withOpacity(0.4),
                offset: const Offset(0, 8),
                blurRadius: 15,
              )
            ],
          ),
          alignment: Alignment.center,
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(
                btnText,
                style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 32),
              ),
              Text(
                "SPIN",
                style: GoogleFonts.luckiestGuy(color: Colors.white.withOpacity(0.9), fontSize: 24),
              ),
              if (canSpin)
                 Padding(
                   padding: const EdgeInsets.only(top: 4),
                   child: Text("PRESS ME", style: GoogleFonts.luckiestGuy(color: Colors.white54, fontSize: 10)),
                 ),
            ],
          ),
        ),
      ),
    );
  }
}

class PointerPainter extends CustomPainter {
  @override
  void paint(Canvas canvas, Size size) {
    final Paint paint = Paint()
      ..color = const Color(0xFFE74C3C)
      ..style = PaintingStyle.fill;

    final Path path = Path();
    path.moveTo(size.width / 2 - 20, 0);
    path.lineTo(size.width / 2 + 20, 0);
    path.lineTo(size.width / 2, size.height);
    path.close();

    // Shadow
    canvas.drawPath(
      path.shift(const Offset(0, 4)), 
      Paint()..color = Colors.black.withOpacity(0.3)..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4)
    );

    canvas.drawPath(path, paint);

    // Gold highlight
    final Paint goldPaint = Paint()
      ..color = const Color(0xFFFFDE00)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 4;
    canvas.drawPath(path, goldPaint);
    
    // Pivot dot
    canvas.drawCircle(Offset(size.width / 2, 5), 5, Paint()..color = Colors.white);
  }

  @override
  bool shouldRepaint(CustomPainter oldDelegate) => false;
}

class WheelPainter extends CustomPainter {
  final List<String> labels;
  final Color Function(String, int) colorProvider;

  WheelPainter({required this.labels, required this.colorProvider});

  @override
  void paint(Canvas canvas, Size size) {
    final double angle = 2 * pi / labels.length;
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2;

    for (int i = 0; i < labels.length; i++) {
      final label = labels[i];
      final paint = Paint()
        ..color = colorProvider(label, i)
        ..style = PaintingStyle.fill;

      // Draw segment
      canvas.drawArc(
        Rect.fromCircle(center: center, radius: radius),
        i * angle - pi / 2,
        angle,
        true,
        paint,
      );

      // White borders between sections
      final borderPaint = Paint()
        ..color = Colors.white.withOpacity(0.3)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2;
      double lineAngle = i * angle - pi / 2;
      canvas.drawLine(
        center,
        Offset(center.dx + radius * cos(lineAngle), center.dy + radius * sin(lineAngle)),
        borderPaint
      );

      // Draw Text
      final textPainter = TextPainter(
        text: TextSpan(
          text: label,
          style: GoogleFonts.luckiestGuy(
            color: Colors.white,
            fontSize: radius * 0.2, // Slightly larger
            shadows: [const Shadow(offset: Offset(2, 2), blurRadius: 4, color: Colors.black45)],
          ),
        ),
        textDirection: TextDirection.ltr,
      );
      textPainter.layout();

      // Rotation and translation for each label
      final textAngle = i * angle + angle / 2 - pi / 2;
      final textRadius = radius * 0.72;
      final x = center.dx + textRadius * cos(textAngle) - textPainter.width / 2;
      final y = center.dy + textRadius * sin(textAngle) - textPainter.height / 2;

      canvas.save();
      canvas.translate(x + textPainter.width / 2, y + textPainter.height / 2);
      canvas.rotate(textAngle + pi / 2);
      canvas.translate(-(x + textPainter.width / 2), -(y + textPainter.height / 2));
      textPainter.paint(canvas, Offset(x, y));
      canvas.restore();
    }

    // Outer rim markers (dots)
    final dotPaint = Paint()..color = Colors.white;
    for (int i = 0; i < 48; i++) {
      double a = i * (2 * pi / 48);
      canvas.drawCircle(
        Offset(center.dx + radius * cos(a), center.dy + radius * sin(a)),
        2,
        dotPaint
      );
    }

    // Outer edge border
    final rimPaint = Paint()
      ..color = Colors.white.withOpacity(0.8)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 3;
    canvas.drawCircle(center, radius, rimPaint);
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}

