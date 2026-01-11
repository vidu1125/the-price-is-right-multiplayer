import 'package:flutter/material.dart';
import 'room_list.dart';
import '../theme/lobby_theme.dart';

class RoomCard extends StatefulWidget {
  const RoomCard({super.key});

  @override
  State<RoomCard> createState() => _RoomCardState();
}

class _RoomCardState extends State<RoomCard> {
  // Form State
  String name = "My Room";
  String visibility = "public";
  String mode = "scoring";
  int maxPlayers = 6;
  bool wagerEnabled = false;

  void handleModeChange(String? newMode) {
    if (newMode == null) return;
    setState(() {
      mode = newMode;
      if (mode == "eliminate") {
        maxPlayers = 4;
      } else {
        if (maxPlayers < 4) maxPlayers = 4;
        if (maxPlayers > 6) maxPlayers = 6;
      }
    });
  }

  void _showCreateRoomModal() {
    showDialog(
      context: context,
      builder: (context) => StatefulBuilder(
        builder: (context, setModalState) {
          return AlertDialog(
            title: const Text("Create New Room"),
            content: SingleChildScrollView(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  TextField(
                    decoration: const InputDecoration(labelText: "Room Name"),
                    onChanged: (val) => name = val,
                  ),
                  DropdownButtonFormField<String>(
                    value: visibility,
                    items: const [
                      DropdownMenuItem(value: "public", child: Text("Public")),
                      DropdownMenuItem(value: "private", child: Text("Private")),
                    ],
                    onChanged: (val) => setModalState(() => visibility = val!),
                    decoration: const InputDecoration(labelText: "Visibility"),
                  ),
                  DropdownButtonFormField<String>(
                    value: mode,
                    items: const [
                      DropdownMenuItem(value: "scoring", child: Text("Scoring")),
                      DropdownMenuItem(value: "eliminate", child: Text("Eliminate")),
                    ],
                    onChanged: (val) {
                      setModalState(() => handleModeChange(val));
                    },
                    decoration: const InputDecoration(labelText: "Game Mode"),
                  ),
                  TextFormField(
                    initialValue: maxPlayers.toString(),
                    enabled: mode != "eliminate",
                    decoration: const InputDecoration(labelText: "Max Players"),
                    keyboardType: TextInputType.number,
                    onChanged: (val) => maxPlayers = int.tryParse(val) ?? 4,
                  ),
                  SwitchListTile(
                    title: const Text("Enable Wager Mode"),
                    value: wagerEnabled,
                    onChanged: (val) => setModalState(() => wagerEnabled = val),
                  )
                ],
              ),
            ),
            actions: [
              TextButton(onPressed: () => Navigator.pop(context), child: const Text("Cancel")),
              ElevatedButton(
                onPressed: () {
                  print("Create room with: $name, $mode, $maxPlayers");
                  Navigator.pop(context);
                },
                child: const Text("Create Room"),
              ),
            ],
          );
        },
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final double titleSize = (screenWidth * 0.025).clamp(24.0, 48.0);
    final double buttonTextSize = (screenWidth * 0.012).clamp(12.0, 20.0);

    return Container(
      decoration: LobbyTheme.roomPanelDecoration,
      padding: const EdgeInsets.all(24),
      child: Column(
        children: [
          Text(
            "ROOM LIST",
            style: LobbyTheme.gameFont(fontSize: titleSize, color: LobbyTheme.yellowGame),
          ),
          const SizedBox(height: 16),
          // Sử dụng Expanded để ListView chiếm trọn không gian trống của khung nền
          Expanded(
            child: RoomList(),
          ),
          const SizedBox(height: 20),
          // Row chứa các button hành động (Reload, Find, Create) đã fix tỷ lệ ở bước trước
          Row(
            children: [
              // Nút Reload
              Expanded(
                flex: 1, // Set to equal flex to match the "identical" request
                child: _actionButton(
                  label: "RELOAD",
                  color: LobbyTheme.yellowReload,
                  textColor: Colors.white,
                  onPressed: () => print("Reloading..."),
                  fontSize: buttonTextSize,
                ),
              ),
              const SizedBox(width: 12),
              
              // Nút Find Room
              Expanded(
                flex: 1, // Set to equal flex
                child: _actionButton(
                  label: "FIND ROOM",
                  color: LobbyTheme.blueAction,
                  textColor: Colors.white,
                  onPressed: () => print("Finding..."),
                  fontSize: buttonTextSize,
                ),
              ),
              const SizedBox(width: 12),
              
              // Nút Create Room
              Expanded(
                flex: 1, // Set to equal flex
                child: _actionButton(
                  label: "+ CREATE ROOM",
                  color: LobbyTheme.greenCreate,
                  textColor: Colors.white,
                  onPressed: () => _showCreateRoomModal(),
                  fontSize: buttonTextSize,
                ),
              ),
            ],
          )
        ],
      ),
    );
  }

  Widget _actionButton({
    required String label,
    required Color color,
    required Color textColor,
    required VoidCallback onPressed,
    double fontSize = 16,
  }) {
    return Container(
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(14),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.2),
            offset: const Offset(0, 4),
            blurRadius: 0, // Shadow cứng (sharp shadow) giống Game
          )
        ],
      ),
      child: ElevatedButton(
        style: ElevatedButton.styleFrom(
          backgroundColor: color,
          foregroundColor: textColor,
          elevation: 0, // Tắt elevation mặc định để dùng BoxShadow của Container
          padding: const EdgeInsets.symmetric(vertical: 18),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(14),
            side: const BorderSide(color: LobbyTheme.primaryDark, width: 3),
          ),
        ).copyWith(
          // Hiệu ứng Hover/Tap nhẹ
          mouseCursor: WidgetStateProperty.all(SystemMouseCursors.click),
        ),
        onPressed: onPressed,
        child: Text(
          label,
          style: LobbyTheme.gameFont(fontSize: fontSize, color: textColor),
        ),
      ),
    );
  }
}