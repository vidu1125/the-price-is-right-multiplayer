import 'package:flutter/material.dart';

class ProductImageContainer extends StatelessWidget {
  final String url;
  
  const ProductImageContainer({super.key, required this.url});

  @override
  Widget build(BuildContext context) {
    return Expanded(
      child: Container(
        margin: const EdgeInsets.symmetric(vertical: 20),
        decoration: BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.circular(20),
          border: Border.all(color: Colors.white24, width: 3),
        ),
        // Placeholder check - if URL is dummy "...", show icon
        child: url == "..." || url.isEmpty 
            ? const Center(child: Icon(Icons.image, size: 80, color: Colors.grey))
            : Image.network(
                url, 
                fit: BoxFit.contain,
                errorBuilder: (ctx, err, stack) => const Center(child: Icon(Icons.broken_image, size: 80, color: Colors.grey)),
              ),
      ),
    );
  }
}
