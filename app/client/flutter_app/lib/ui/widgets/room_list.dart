import 'package:flutter/material.dart';
import '../theme/lobby_theme.dart';

class Room {
  final int id;
  final String name;
  final String status;
  Room({required this.id, required this.name, required this.status});
}

class RoomList extends StatelessWidget {
  final List<Room> rooms = [
    Room(id: 1, name: "The Price is Right (A)", status: "Waiting"),
    Room(id: 2, name: "Challenge: High Stakes (B)", status: "In Game"),
    Room(id: 3, name: "Casual Fun Room (C)", status: "Waiting"),
    Room(id: 4, name: "Vietnamese Pricing Game (D)", status: "Waiting"),
    Room(id: 5, name: "Max Bid Only (E)", status: "Waiting"),
    Room(id: 6, name: "Quick Play - Open Slot (F)", status: "Waiting"),
    Room(id: 7, name: "The Big Reveal (G)", status: "In Game"),
    Room(id: 8, name: "Midnight Madness (H)", status: "Waiting"),
    Room(id: 999, name: "[TEST] Simulation Room", status: "Waiting"),
  ];

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final double contentFontSize = (screenWidth * 0.015).clamp(14.0, 24.0);
    final double buttonFontSize = (screenWidth * 0.014).clamp(14.0, 22.0);
    
    // Show all rooms - scroll if more than ~8 visible
    
    return ListView.separated(
      padding: const EdgeInsets.symmetric(vertical: 8),
      itemCount: rooms.length, // Show all available rooms
      separatorBuilder: (context, index) => const SizedBox(height: 8),
      itemBuilder: (context, index) {
        final room = rooms[index];
        bool isInGame = room.status == "In Game";


        return Container(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
          decoration: LobbyTheme.tableRowDecoration,
          child: Row(
            children: [
              // ID column - width for 5 digits
              SizedBox(
                width: 70,
                child: Text(
                  room.id.toString(),
                  style: LobbyTheme.tableContentFont.copyWith(fontSize: contentFontSize),
                ),
              ),
              
              // NAME column - expanded
              Expanded(
                child: Text(
                  room.name,
                  style: LobbyTheme.tableContentFont.copyWith(
                    fontWeight: FontWeight.bold,
                    fontSize: contentFontSize
                  ),
                  overflow: TextOverflow.ellipsis,
                ),
              ),
              
              const SizedBox(width: 20), // Spacing before button
              
              // JOIN/LOCKED button - fixed size
              SizedBox(
                width: 110, // Increased from 100 to fit "LOCKED" text fully
                height: 42,
                child: ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: isInGame ? Colors.grey : const Color(0xFF66BB6A),
                    padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(10),
                      side: const BorderSide(color: LobbyTheme.primaryDark, width: 2),
                    ),
                  ),
                  onPressed: isInGame 
                    ? null 
                    : () {
                        Navigator.pushNamed(context, '/room', arguments: {'roomId': room.id});
                      },
                  child: Text(
                    isInGame ? "LOCK" : "JOIN",
                    style: LobbyTheme.gameFont(fontSize: buttonFontSize, color: Colors.white),
                  ),
                ),
              ),
            ],
          ),
        );
      },
    );
  }
}