import 'package:flutter/material.dart';

import '../theme/lobby_theme.dart';

class AppTitle extends StatelessWidget {
  const AppTitle({super.key});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 20),
      child: Text(
        "THE PRICE IS RIGHT",
        style: LobbyTheme.gameFont(fontSize: 30, color: LobbyTheme.yellowGame),
      ),
    );
  }
}