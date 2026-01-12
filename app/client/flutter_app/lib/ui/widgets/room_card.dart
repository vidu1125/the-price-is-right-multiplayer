import 'package:flutter/material.dart';
import 'room_list.dart';
import '../theme/lobby_theme.dart';
import '../../services/service_locator.dart';

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
      barrierDismissible: false,
      builder: (context) => StatefulBuilder(
        builder: (context, setModalState) {
          return Dialog(
            backgroundColor: Colors.transparent,
            insetPadding: const EdgeInsets.symmetric(horizontal: 20),
            child: Container(
              width: 400, // Fixed max width for better aesthetic
              decoration: BoxDecoration(
                color: Colors.white,
                borderRadius: BorderRadius.circular(24),
                border: Border.all(color: LobbyTheme.primaryDark, width: 5),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withOpacity(0.5),
                    offset: const Offset(0, 10),
                    blurRadius: 20,
                  )
                ],
              ),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  // --- HEADER ---
                  Container(
                    width: double.infinity,
                    padding: const EdgeInsets.symmetric(vertical: 16),
                    decoration: const BoxDecoration(
                      color: LobbyTheme.yellowGame,
                      borderRadius: BorderRadius.only(
                        topLeft: Radius.circular(18),
                        topRight: Radius.circular(18),
                      ),
                      border: Border(
                        bottom: BorderSide(color: LobbyTheme.primaryDark, width: 4),
                      ),
                    ),
                    child: Center(
                      child: Text(
                        "CREATE ROOM",
                        style: LobbyTheme.gameFont(
                          fontSize: 28,
                          color: Colors.white,
                        ).copyWith(
                          shadows: [
                            const Shadow(offset: Offset(2, 2), color: LobbyTheme.primaryDark),
                            const Shadow(offset: Offset(-1, -1), color: LobbyTheme.primaryDark), // Extra thick outline
                            const Shadow(offset: Offset(-1, 1), color: LobbyTheme.primaryDark),
                            const Shadow(offset: Offset(1, -1), color: LobbyTheme.primaryDark),
                          ]
                        ),
                      ),
                    ),
                  ),

                  // --- BODY ---
                  Flexible(
                    child: SingleChildScrollView(
                      padding: const EdgeInsets.all(24),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          _buildLabel("ROOM NAME"),
                          const SizedBox(height: 8),
                          _buildTextField(
                            hint: "Enter room name...",
                            onChanged: (val) => name = val,
                            icon: Icons.meeting_room_rounded,
                          ),
                          
                          const SizedBox(height: 16),
                          
                          Row(
                            children: [
                              Expanded(
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    _buildLabel("VISIBILITY"),
                                    const SizedBox(height: 8),
                                    _buildDropdown(
                                      value: visibility,
                                      items: const ["public", "private"],
                                      onChanged: (val) => setModalState(() => visibility = val!),
                                      icon: Icons.visibility_rounded,
                                    ),
                                  ],
                                ),
                              ),
                              const SizedBox(width: 16),
                              Expanded(
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    _buildLabel("MODE"),
                                    const SizedBox(height: 8),
                                    _buildDropdown(
                                      value: mode,
                                      items: const ["scoring", "eliminate"],
                                      onChanged: (val) => setModalState(() => handleModeChange(val)),
                                      icon: Icons.sports_esports_rounded,
                                    ),
                                  ],
                                ),
                              ),
                            ],
                          ),

                          const SizedBox(height: 16),
                          
                          Row(
                            children: [
                              Expanded(
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    _buildLabel("MAX PLAYERS"),
                                    const SizedBox(height: 8),
                                    _buildTextField(
                                      initialValue: maxPlayers.toString(),
                                      enabled: mode != "eliminate",
                                      keyboardType: TextInputType.number,
                                      onChanged: (val) => maxPlayers = int.tryParse(val) ?? 4,
                                      icon: Icons.group_rounded,
                                    ),
                                  ],
                                ),
                              ),
                              const SizedBox(width: 16),
                              Expanded(
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    _buildLabel("WAGER"),
                                    const SizedBox(height: 8),
                                    Container(
                                      height: 54, // Match height of inputs
                                      decoration: BoxDecoration(
                                        color: Colors.grey[100],
                                        borderRadius: BorderRadius.circular(12),
                                        border: Border.all(color: LobbyTheme.primaryDark, width: 2),
                                      ),
                                      child: Row(
                                        mainAxisAlignment: MainAxisAlignment.center,
                                        children: [
                                          Text(
                                            wagerEnabled ? "ON" : "OFF",
                                            style: TextStyle(
                                              fontFamily: 'Parkinsans',
                                              fontWeight: FontWeight.bold,
                                              color: wagerEnabled ? Colors.green : Colors.grey,
                                              fontSize: 16
                                            ),
                                          ),
                                          Switch(
                                            value: wagerEnabled,
                                            activeColor: LobbyTheme.greenCreate,
                                            onChanged: (val) => setModalState(() => wagerEnabled = val),
                                          ),
                                        ],
                                      ),
                                    )
                                  ],
                                ),
                              ),
                            ],
                          ),
                        ],
                      ),
                    ),
                  ),

                  // --- FOOTER / ACTIONS ---
                  Container(
                    padding: const EdgeInsets.only(left: 24, right: 24, bottom: 24),
                    child: Row(
                      children: [
                        Expanded(
                          child: _modalButton(
                            label: "CANCEL",
                            color: Colors.grey[400]!, // Or red
                            textColor: LobbyTheme.primaryDark,
                            onPressed: () => Navigator.pop(context),
                          ),
                        ),
                        const SizedBox(width: 16),
                        Expanded(
                          child: _modalButton(
                            label: "CREATE",
                            color: LobbyTheme.greenCreate,
                            textColor: Colors.white,
                            onPressed: () async {
                               final res = await ServiceLocator.roomService.createRoom(
                                  name: name,
                                  visibility: visibility,
                                  mode: mode,
                                  maxPlayers: maxPlayers,
                                  wager: wagerEnabled
                              );
                              
                              if (context.mounted) {
                                  Navigator.pop(context); 
                                  if (res['success'] == true) {
                                       Navigator.pushNamed(context, '/room', arguments: {'roomId': res['roomId']});
                                  } else {
                                      ScaffoldMessenger.of(context).showSnackBar(
                                          SnackBar(content: Text("Failed to create room: ${res['error']}"))
                                      );
                                  }
                              }
                            },
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    );
  }

  // --- Helper Widgets for the Modal ---

  Widget _buildLabel(String text) {
    return Text(
      text,
      style: const TextStyle(
        fontFamily: 'LuckiestGuy',
        color: LobbyTheme.primaryDark,
        fontSize: 14,
        letterSpacing: 0.5,
      ),
    );
  }

  Widget _buildTextField({
    String? hint,
    String? initialValue,
    bool enabled = true,
    TextInputType? keyboardType,
    required Function(String) onChanged,
    required IconData icon,
  }) {
    return Container(
      decoration: BoxDecoration(
        color: enabled ? Colors.white : Colors.grey[200],
        borderRadius: BorderRadius.circular(12),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            offset: const Offset(0, 4),
            blurRadius: 0,
          )
        ]
      ),
      child: TextFormField(
        initialValue: initialValue,
        enabled: enabled,
        keyboardType: keyboardType,
        onChanged: onChanged,
        style: const TextStyle(fontFamily: 'Parkinsans', fontWeight: FontWeight.bold, fontSize: 16),
        decoration: InputDecoration(
          hintText: hint,
          prefixIcon: Icon(icon, color: LobbyTheme.primaryDark),
          filled: true,
          fillColor: enabled ? Colors.grey[100] : Colors.grey[300],
          contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
          enabledBorder: OutlineInputBorder(
            borderRadius: BorderRadius.circular(12),
            borderSide: const BorderSide(color: LobbyTheme.primaryDark, width: 2),
          ),
          focusedBorder: OutlineInputBorder(
            borderRadius: BorderRadius.circular(12),
            borderSide: const BorderSide(color: LobbyTheme.blueAction, width: 2),
          ),
          disabledBorder: OutlineInputBorder(
            borderRadius: BorderRadius.circular(12),
            borderSide: const BorderSide(color: Colors.grey, width: 2),
          ),
        ),
      ),
    );
  }

  Widget _buildDropdown({
    required String value,
    required List<String> items,
    required Function(String?) onChanged,
    required IconData icon,
  }) {
    return Container(
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(12),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            offset: const Offset(0, 4),
            blurRadius: 0,
          )
        ]
      ),
      child: DropdownButtonFormField<String>(
        value: value,
        onChanged: onChanged,
        items: items.map((e) => DropdownMenuItem(
          value: e,
          child: Text(e.toUpperCase(), style: const TextStyle(
            fontFamily: 'Parkinsans', fontWeight: FontWeight.bold, fontSize: 14
          )),
        )).toList(),
        decoration: InputDecoration(
          prefixIcon: Icon(icon, color: LobbyTheme.primaryDark),
          filled: true,
          fillColor: Colors.grey[100],
          contentPadding: const EdgeInsets.symmetric(horizontal: 12, vertical: 14),
          enabledBorder: OutlineInputBorder(
            borderRadius: BorderRadius.circular(12),
            borderSide: const BorderSide(color: LobbyTheme.primaryDark, width: 2),
          ),
          focusedBorder: OutlineInputBorder(
            borderRadius: BorderRadius.circular(12),
            borderSide: const BorderSide(color: LobbyTheme.blueAction, width: 2),
          ),
        ),
      ),
    );
  }

  Widget _modalButton({
    required String label,
    required Color color,
    required Color textColor,
    required VoidCallback onPressed,
  }) {
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        backgroundColor: color,
        foregroundColor: textColor,
        padding: const EdgeInsets.symmetric(vertical: 16),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(12),
          side: const BorderSide(color: LobbyTheme.primaryDark, width: 2),
        ),
        elevation: 0,
      ),
      onPressed: onPressed,
      child: Text(
        label,
        style: TextStyle(
          fontFamily: 'LuckiestGuy',
          fontSize: 18,
          color: textColor,
          letterSpacing: 1,
          shadows: color != Colors.white ? [
             Shadow(offset: const Offset(1, 1), color: Colors.black.withOpacity(0.2))
          ] : null,
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final double titleSize = (screenWidth * 0.028).clamp(26.0, 52.0); // Increased from 0.025
    final double buttonTextSize = (screenWidth * 0.015).clamp(14.0, 24.0); // Increased from 0.012, 12-20

    return LayoutBuilder(
      builder: (context, constraints) {
        // Responsive padding - reduced max for more compact display
        final double containerPadding = (constraints.maxWidth * 0.03).clamp(16.0, 24.0);
        
        return Center(
          child: Container(
            // Increase to 98% for maximum width
            width: constraints.maxWidth * 0.98,
            decoration: LobbyTheme.roomPanelDecoration,
            padding: EdgeInsets.all(containerPadding), // Responsive padding
            child: Column(
              children: [
                Text(
                  "ROOM LIST",
                  style: LobbyTheme.gameFont(fontSize: titleSize, color: LobbyTheme.yellowGame),
                ),
                const SizedBox(height: 12), // Giảm từ 16
                // Dùng Flexible để RoomList tự động điều chỉnh chiều cao
                Flexible(
                  child: RoomList(),
                ),
                const SizedBox(height: 10), // Giảm từ 12
                // Row containing action buttons (Reload, Find, Create)
                Row(
                  children: [
                    // Reload button
                    Expanded(
                      flex: 4, // Make Reload and Find larger (flex 4)
                      child: _actionButton(
                        label: "RELOAD",
                        color: LobbyTheme.yellowReload,
                        textColor: Colors.white,
                        onPressed: () => print("Reloading..."),
                        fontSize: buttonTextSize,
                        verticalPadding: 24, // Increased from 22
                      ),
                    ),
                    const SizedBox(width: 12),
                    
                    // Find Room button
                    Expanded(
                      flex: 4, // Make Reload and Find larger
                      child: _actionButton(
                        label: "FIND ROOM",
                        color: LobbyTheme.blueAction,
                        textColor: Colors.white,
                        onPressed: () => print("Finding..."),
                        fontSize: buttonTextSize,
                        verticalPadding: 24, // Increased from 22
                      ),
                    ),
                    const SizedBox(width: 12),
                    
                    // Create Room button
                    Expanded(
                      flex: 5, // Create Room slightly bigger or similar
                      child: _actionButton(
                        label: "+ CREATE ROOM",
                        color: LobbyTheme.greenCreate,
                        textColor: Colors.white,
                        onPressed: () => _showCreateRoomModal(),
                        fontSize: buttonTextSize,
                        verticalPadding: 24, // Increased from 22
                      ),
                    ),
                  ],
                )
              ],
            ),
          ),
        );
      }
    );
  }

  Widget _actionButton({
    required String label,
    required Color color,
    required Color textColor,
    required VoidCallback onPressed,
    double fontSize = 16,
    double verticalPadding = 18,
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
          padding: EdgeInsets.symmetric(vertical: verticalPadding),
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