import 'package:flutter/material.dart';
import '../theme/lobby_theme.dart';
import '../../models/room.dart';
import '../../services/service_locator.dart';

class RoomList extends StatelessWidget {
  final List<Room> rooms;

  const RoomList({super.key, required this.rooms});

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
        bool isPlaying = room.status.toLowerCase() != "waiting";
        bool isFull = room.currentPlayerCount >= room.maxPlayers;
        bool isLocked = isPlaying || isFull;

        String buttonText = "JOIN";
        if (isPlaying) buttonText = "PLAYING";
        else if (isFull) buttonText = "FULL";

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
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      room.name,
                      style: LobbyTheme.tableContentFont.copyWith(
                        fontWeight: FontWeight.bold,
                        fontSize: contentFontSize
                      ),
                      overflow: TextOverflow.ellipsis,
                    ),
                    Text(
                      "${room.currentPlayerCount}/${room.maxPlayers} Players • ${room.mode.toUpperCase()}",
                      style: LobbyTheme.tableContentFont.copyWith(
                        fontSize: contentFontSize * 0.7,
                        color: Colors.white70
                      ),
                    ),
                  ],
                ),
              ),
              
              const SizedBox(width: 20), // Spacing before button
              
              // JOIN/LOCKED button - fixed size
              SizedBox(
                width: 110, // Increased from 100 to fit "LOCKED" text fully
                height: 42,
                child: ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: isLocked ? Colors.grey : const Color(0xFF66BB6A),
                    padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(10),
                      side: const BorderSide(color: LobbyTheme.primaryDark, width: 2),
                    ),
                  ),
                  onPressed: isLocked 
                    ? null 
                    : () async {
                        try {
                           final joinedRoom = await ServiceLocator.roomService.joinRoom(room.id);
                           if (context.mounted && joinedRoom != null) {
                               Navigator.pushNamed(context, '/room', arguments: {'room': joinedRoom, 'initialIsHost': false});
                           }
                        } catch (e) {
                           if (context.mounted) {
                               ScaffoldMessenger.of(context).showSnackBar(
                                   SnackBar(content: Text("Failed to join: $e"), backgroundColor: Colors.red),
                               );
                           }
                        }
                      },
                  child: Text(
                    buttonText,
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