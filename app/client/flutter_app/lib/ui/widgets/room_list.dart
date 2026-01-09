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
  ];

  @override
  Widget build(BuildContext context) {
    return ListView.separated(
      padding: const EdgeInsets.symmetric(vertical: 8),
      itemCount: rooms.length,
      separatorBuilder: (context, index) => const SizedBox(height: 8), // Khoảng cách giữa các hàng
      itemBuilder: (context, index) {
        final room = rooms[index];
        bool isInGame = room.status == "In Game";

        return Container(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
          decoration: LobbyTheme.tableRowDecoration, // Dùng decoration từ theme
          child: Row(
            children: [
              SizedBox(
                width: 40,
                child: Text(room.id.toString(), style: LobbyTheme.tableContentFont),
              ),
              Expanded(
                child: Text(
                  room.name,
                  style: LobbyTheme.tableContentFont.copyWith(fontWeight: FontWeight.bold),
                  overflow: TextOverflow.ellipsis,
                ),
              ),
              SizedBox(
                width: 80,
                child: Text(
                  room.status,
                  style: LobbyTheme.tableContentFont.copyWith(
                    color: isInGame ? Colors.orangeAccent : Colors.greenAccent,
                  ),
                ),
              ),
              ElevatedButton(
                style: ElevatedButton.styleFrom(
                  backgroundColor: isInGame ? Colors.grey : const Color(0xFF66BB6A),
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 0),
                  minimumSize: const Size(60, 30),
                  shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
                ),
                onPressed: isInGame 
                  ? null 
                  : () {
                      Navigator.pushNamed(context, '/room');
                    },
                child: Text(isInGame ? "Locked" : "Join", style: const TextStyle(fontSize: 12)),
              ),
            ],
          ),
        );
      },
    );
  }
}