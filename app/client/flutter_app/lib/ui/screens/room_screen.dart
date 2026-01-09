import 'package:flutter/material.dart';
import '../theme/lobby_theme.dart';
import '../widgets/user_card.dart';

class WaitingRoomScreen extends StatefulWidget {
  const WaitingRoomScreen({super.key});

  @override
  State<WaitingRoomScreen> createState() => _WaitingRoomScreenState();
}

class _WaitingRoomScreenState extends State<WaitingRoomScreen> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          // Background
          Container(
            width: double.infinity,
            height: double.infinity,
            decoration: const BoxDecoration(
              image: DecorationImage(
                image: AssetImage('assets/images/waitingroom.png'),
                fit: BoxFit.cover,
              ),
            ),
          ),

          // User Card (Top Left)
          const Positioned(
            top: 50,
            left: 30,
            child: UserCard(),
          ),

          // Main Content
          SafeArea(
            child: Column(
              children: [
                _buildHeader(),
                Expanded(
                  child: Padding(
                    padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 20),
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        // CỘT 1: GAME RULES
                        Expanded(flex: 2, child: _buildGameRulesPanel()),
                        const SizedBox(width: 30),
                        
                        // CỘT 2: MEMBER LIST
                        Expanded(flex: 3, child: _buildMemberListPanel()),
                        const SizedBox(width: 30),
                        
                        // CỘT 3: ACTIONS
                        SizedBox(width: 180, child: _buildActionButtons()),
                      ],
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHeader() {
    return const Padding(
      padding: EdgeInsets.only(top: 20),
      child: Text(
        "THE PRICE IS RIGHT",
        style: TextStyle(
          fontFamily: 'LuckiestGuy',
          fontSize: 40,
          color: RoomTheme.primaryDark,
          shadows: [
            Shadow(offset: Offset(2, 2), color: Colors.white),
            Shadow(offset: Offset(4, 4), color: Color(0xFFFFAA00)),
          ],
        ),
      ),
    );
  }

  Widget _buildGameRulesPanel() {
    return Container(
      decoration: RoomTheme.panelDecoration,
      padding: const EdgeInsets.all(20),
      child: Column(
        children: [
          const Text("GAME RULES", style: RoomTheme.titleStyle),
          const SizedBox(height: 20),
          _ruleRow("Max Players", "5"),
          _ruleRow("Visibility", "Public"),
          _ruleRow("Mode", "Scoring"),
          _ruleRow("Wager Mode", "ON"),
        ],
      ),
    );
  }

  Widget _buildMemberListPanel() {
    return Container(
      decoration: RoomTheme.panelDecoration,
      padding: const EdgeInsets.all(20),
      child: Column(
        children: [
          const Text("FUN ROOM", style: RoomTheme.titleStyle),
          const SizedBox(height: 20),
          Expanded(
            child: GridView.builder(
              gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: 3,
                crossAxisSpacing: 15,
                mainAxisSpacing: 15,
                childAspectRatio: 0.8,
              ),
              itemCount: 6,
              itemBuilder: (context, index) => _buildPlayerCard(index),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildPlayerCard(int index) {
    return Container(
      decoration: BoxDecoration(
        color: const Color(0xFFE3E9F2),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: RoomTheme.primaryDark, width: 3),
      ),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const CircleAvatar(radius: 30, backgroundColor: Colors.white),
          const SizedBox(height: 8),
          const Text("Player", style: RoomTheme.cardNameStyle),
          const SizedBox(height: 5),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
            decoration: BoxDecoration(
              color: RoomTheme.accentGreen,
              borderRadius: BorderRadius.circular(15),
            ),
            child: const Text("READY", style: RoomTheme.statusTagStyle),
          )
        ],
      ),
    );
  }

  Widget _buildActionButtons() {
    return Column(
      children: [
        _actionButton("START GAME", RoomTheme.accentYellow, RoomTheme.primaryDark),
        const SizedBox(height: 10),
        _actionButton("INVITE FRIENDS", RoomTheme.accentGreen, Colors.white),
        const SizedBox(height: 10),
        _actionButton("LEAVE ROOM", RoomTheme.accentRed, Colors.white),
      ],
    );
  }

  Widget _actionButton(String label, Color bg, Color textCol) {
    return SizedBox(
      width: double.infinity,
      child: ElevatedButton(
        style: ElevatedButton.styleFrom(
          backgroundColor: bg,
          side: const BorderSide(color: RoomTheme.primaryDark, width: 2),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
          padding: const EdgeInsets.symmetric(vertical: 15),
        ),
        onPressed: () {},
        child: Text(label, style: TextStyle(fontFamily: 'LuckiestGuy', color: textCol)),
      ),
    );
  }

  Widget _ruleRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(fontFamily: 'LuckiestGuy', color: Color(0xFF29B6F6), fontSize: 18)),
          Text(value, style: const TextStyle(fontFamily: 'LuckiestGuy', color: RoomTheme.accentYellow, fontSize: 22)),
        ],
      ),
    );
  }
}