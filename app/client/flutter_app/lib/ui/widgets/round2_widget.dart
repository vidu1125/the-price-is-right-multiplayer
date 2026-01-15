import 'package:flutter/material.dart';
import 'badge_widget.dart';
import 'game_button.dart';

class Round2Widget extends StatefulWidget {
  final Map productData;
  final Function(int) onSubmitBid;

  const Round2Widget({super.key, required this.productData, required this.onSubmitBid});

  @override
  _Round2WidgetState createState() => _Round2WidgetState();
}

class _Round2WidgetState extends State<Round2Widget> {
  final TextEditingController _controller = TextEditingController();
  bool isSubmitted = false;

  @override
  void didUpdateWidget(Round2Widget oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.productData['product_idx'] != widget.productData['product_idx']) {
      setState(() {
        _controller.clear();
        isSubmitted = false;
      });
    }
  }

  void _submit() {
    if (isSubmitted) return;
    final value = int.tryParse(_controller.text);
    if (value != null) {
      setState(() => isSubmitted = true);
      widget.onSubmitBid(value);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            BadgeWidget(
              text: "PRODUCT ${(widget.productData['product_idx'] ?? 0) + 1}/${widget.productData['total_products'] ?? 5}", 
              color: const Color(0xFFFFDE00)
            ),
          ],
        ),
        const SizedBox(height: 30),
        
        // Product Question
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 20),
          child: Text(
            widget.productData['question'] ?? "Guess the price!", 
            style: const TextStyle(fontFamily: 'LuckiestGuy', fontSize: 32, color: Colors.white), 
            textAlign: TextAlign.center
          ),
        ),
        
        // Product Image (if available) - Placeholder for now
        if (widget.productData['product_image'] != null)
           Padding(
             padding: const EdgeInsets.all(20.0),
             child: Image.network(widget.productData['product_image'], height: 200),
           ),

        const Spacer(),
        
        // Input Area
        Row(
          children: [
            Expanded(
              child: TextField(
                controller: _controller,
                keyboardType: TextInputType.number,
                enabled: !isSubmitted,
                decoration: InputDecoration(
                  fillColor: Colors.white, filled: true,
                  hintText: "Enter price...",
                  border: OutlineInputBorder(borderRadius: BorderRadius.circular(15), borderSide: const BorderSide(width: 3)),
                ),
              ),
            ),
            const SizedBox(width: 15),
            GameButton(
              text: isSubmitted ? "WAITING..." : "SUBMIT", 
              onPressed: isSubmitted ? () {} : _submit, 
              color: isSubmitted ? Colors.grey : const Color(0xFF2ECC71)
            ),
          ],
        ),
        const SizedBox(height: 20),
      ],
    );
  }
}
