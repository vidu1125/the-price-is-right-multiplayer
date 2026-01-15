// lib/ui/widgets/invitation_dialog.dart
import 'dart:async';
import 'package:flutter/material.dart';
import '../../services/service_locator.dart';
import '../theme/lobby_theme.dart';

class InvitationDialog extends StatefulWidget {
  final int senderId;
  final String senderName;
  final int roomId;
  final String roomName;

  const InvitationDialog({
    super.key,
    required this.senderId,
    required this.senderName,
    required this.roomId,
    required this.roomName,
  });

  @override
  State<InvitationDialog> createState() => _InvitationDialogState();
}

class _InvitationDialogState extends State<InvitationDialog> with SingleTickerProviderStateMixin {
  late Timer _timer;
  int _timeLeft = 60;
  bool _isJoining = false;

  @override
  void initState() {
    super.initState();
    _timer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (!mounted) return;
      setState(() {
        if (_timeLeft > 0) {
          _timeLeft--;
        } else {
          _timer.cancel();
          Navigator.of(context).pop(); // Auto dismiss
        }
      });
    });
  }

  @override
  void dispose() {
    _timer.cancel();
    super.dispose();
  }

  Future<void> _accept() async {
      setState(() => _isJoining = true);
      try {
          // Join the room
          final room = await ServiceLocator.roomService.joinRoom(widget.roomId);
          if (!mounted) return;
          
          if (room != null) {
              Navigator.of(context).pop(); // Close dialog
              // Navigate to RoomScreen
              // Note: We need to use the context from the Lobby or whatever parent to navigate.
              // Assuming standard navigation:
              Navigator.of(context).pushNamed('/room', arguments: room);
          }
      } catch (e) {
          if (mounted) {
              setState(() => _isJoining = false);
              ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("Failed to join: $e")));
          }
      }
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
       backgroundColor: LobbyTheme.primaryDark,
       shape: RoundedRectangleBorder(
           borderRadius: BorderRadius.circular(16),
           side: const BorderSide(color: LobbyTheme.yellowGame, width: 3)
       ),
       child: Padding(
           padding: const EdgeInsets.all(24),
           child: Column(
               mainAxisSize: MainAxisSize.min,
               children: [
                   const Text("GAME INVITATION", 
                       style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 24, color: LobbyTheme.yellowGame)
                   ),
                   const SizedBox(height: 20),
                   const CircleAvatar(
                       radius: 30,
                       backgroundColor: Colors.white24,
                       child: Icon(Icons.mail, size: 30, color: Colors.white),
                   ),
                   const SizedBox(height: 20),
                   RichText(
                       textAlign: TextAlign.center,
                       text: TextSpan(
                           style: const TextStyle(color: Colors.white, fontSize: 16),
                           children: [
                               const TextSpan(text: "Your friend "),
                               TextSpan(text: widget.senderName, style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.greenAccent)),
                               const TextSpan(text: "\ninvited you to join room\n"),
                               TextSpan(text: widget.roomName, style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.amber)),
                           ]
                       )
                   ),
                   const SizedBox(height: 30),
                   if (_isJoining)
                       const CircularProgressIndicator(color: LobbyTheme.yellowGame)
                   else
                       Column(
                           children: [
                               Text(
                                   "Expires in 00:${_timeLeft.toString().padLeft(2, '0')}", 
                                   style: const TextStyle(color: Colors.redAccent, fontSize: 18, fontWeight: FontWeight.bold, fontFamily: 'LuckiestGuy')
                               ),
                               const SizedBox(height: 15),
                               Row(
                                   mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                                   children: [
                                       ElevatedButton(
                                           onPressed: () => Navigator.of(context).pop(),
                                           style: ElevatedButton.styleFrom(
                                               backgroundColor: Colors.red,
                                               padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12)
                                           ),
                                           child: const Text("Decline", style: TextStyle(color: Colors.white)),
                                       ),
                                       ElevatedButton(
                                           onPressed: _accept,
                                           style: ElevatedButton.styleFrom(
                                               backgroundColor: Colors.green,
                                               padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 12)
                                           ),
                                           child: const Text("ACCEPT", style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 16)),
                                       ),
                                   ]
                               ),
                           ],
                       )
               ]
           )
       )
    );
  }
}
