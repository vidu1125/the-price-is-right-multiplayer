import 'dart:async';
import 'package:flutter/material.dart';
import '../../services/service_locator.dart';
import '../theme/lobby_theme.dart';

class InvitePlayersDialog extends StatefulWidget {
  final int roomId;

  const InvitePlayersDialog({
    super.key,
    required this.roomId,
  });

  @override
  State<InvitePlayersDialog> createState() => _InvitePlayersDialogState();
}

class _InvitePlayersDialogState extends State<InvitePlayersDialog> {
  final TextEditingController _idController = TextEditingController();
  List<dynamic> _onlineFriends = [];
  bool _isLoading = false;
  final Set<int> _invitedIds = {};

  @override
  void initState() {
    super.initState();
    _fetchOnlineFriends();
  }

  @override
  void dispose() {
    _idController.dispose();
    super.dispose();
  }

  Future<void> _fetchOnlineFriends() async {
    setState(() => _isLoading = true);
    final res = await ServiceLocator.friendService.getFriendList();
    if (mounted) {
      setState(() {
        _isLoading = false;
        if (res["success"] == true) {
          final allFriends = res["friends"] as List<dynamic>? ?? [];
          // Filter online friends who are not currently playing? 
          // Backend might not strictly block invites to playing users, but UI can hint.
          // For now just show all online friends.
          _onlineFriends = allFriends.where((f) => f["online"] == true).toList();
        }
      });
    }
  }

  Future<void> _inviteUser(int userId, [String? name]) async {
    if (_invitedIds.contains(userId)) return;

    final success = await ServiceLocator.roomService.inviteFriend(userId, widget.roomId);
    if (mounted) {
      if (success) {
        setState(() => _invitedIds.add(userId));
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text("Invitation sent to ${name ?? 'User $userId'}!"))
        );
      } else {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text("Failed to send invitation."))
        );
      }
    }
  }

  void _onManualInvite() {
    final text = _idController.text.trim();
    if (text.isEmpty) return;
    
    final id = int.tryParse(text);
    if (id == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text("Please enter a valid numeric User ID"))
      );
      return;
    }
    _inviteUser(id);
    _idController.clear();
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
       backgroundColor: LobbyTheme.backgroundNavy,
       shape: RoundedRectangleBorder(
           borderRadius: BorderRadius.circular(16),
           side: const BorderSide(color: LobbyTheme.yellowGame, width: 3),
       ),
       child: Container(
         width: 450,
         padding: const EdgeInsets.all(24),
         child: Column(
           mainAxisSize: MainAxisSize.min,
           children: [
             // Header
             Row(
               mainAxisAlignment: MainAxisAlignment.spaceBetween,
               children: [
                 const Text("INVITE PLAYERS", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 24, color: LobbyTheme.yellowGame)),
                 IconButton(
                   icon: const Icon(Icons.close, color: Colors.white), 
                   onPressed: () => Navigator.pop(context),
                 )
               ],
             ),
             const SizedBox(height: 20),

             // Manual Invite Section
             Container(
               padding: const EdgeInsets.all(15),
               decoration: BoxDecoration(
                 color: Colors.white.withOpacity(0.05),
                 borderRadius: BorderRadius.circular(10),
                 border: Border.all(color: Colors.white10),
               ),
               child: Column(
                 crossAxisAlignment: CrossAxisAlignment.start,
                 children: [
                   const Text("Invite by ID", style: TextStyle(color: Color(0xFF29B6F6), fontWeight: FontWeight.bold)),
                   const SizedBox(height: 10),
                   Row(
                     children: [
                       Expanded(
                         child: TextField(
                           controller: _idController,
                           style: const TextStyle(color: Colors.white),
                           decoration: InputDecoration(
                             hintText: "Enter User ID...",
                             hintStyle: const TextStyle(color: Colors.white30),
                             filled: true,
                             fillColor: Colors.black26,
                             border: OutlineInputBorder(borderRadius: BorderRadius.circular(8)),
                             contentPadding: const EdgeInsets.symmetric(horizontal: 10, vertical: 0),
                           ),
                           keyboardType: TextInputType.number,
                         ),
                       ),
                       const SizedBox(width: 10),
                       ElevatedButton(
                         onPressed: _onManualInvite,
                         style: ElevatedButton.styleFrom(
                           backgroundColor: LobbyTheme.yellowGame,
                           foregroundColor: Colors.black,
                         ),
                         child: const Text("INVITE"),
                       )
                     ],
                   )
                 ],
               ),
             ),
             const SizedBox(height: 20),

             // Online Friends List
             Align(
               alignment: Alignment.centerLeft,
               child: Text("Online Friends (${_onlineFriends.length})", style: const TextStyle(color: Color(0xFF29B6F6), fontWeight: FontWeight.bold))
             ),
             const SizedBox(height: 10),
             
             Container(
               height: 250,
               decoration: BoxDecoration(
                 color: Colors.black26,
                 borderRadius: BorderRadius.circular(10),
                 border: Border.all(color: Colors.white12),
               ),
               child: _isLoading 
                 ? const Center(child: CircularProgressIndicator())
                 : _onlineFriends.isEmpty
                   ? const Center(child: Text("No friends online.", style: TextStyle(color: Colors.white38)))
                   : ListView.separated(
                       padding: const EdgeInsets.all(10),
                       itemCount: _onlineFriends.length,
                       separatorBuilder: (_, __) => const SizedBox(height: 8),
                       itemBuilder: (context, index) {
                         final friend = _onlineFriends[index];
                         final id = friend['id'];
                         final isInvited = _invitedIds.contains(id);
                         
                         return Container(
                           padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
                           decoration: BoxDecoration(
                             color: Colors.white.withOpacity(0.05),
                             borderRadius: BorderRadius.circular(8),
                           ),
                           child: Row(
                             children: [
                               // Avatar
                               Container(
                                 width: 32, height: 32,
                                 decoration: BoxDecoration(
                                   shape: BoxShape.circle,
                                   border: Border.all(color: LobbyTheme.yellowGame),
                                   image: DecorationImage(
                                     image: (friend['avatar'] != null && friend['avatar'].startsWith("http")) 
                                         ? NetworkImage(friend['avatar']) 
                                         : const AssetImage("assets/images/default-mushroom.jpg") as ImageProvider,
                                     fit: BoxFit.cover,
                                   ),
                                 ),
                               ),
                               const SizedBox(width: 10),
                               // Info
                               Expanded(
                                 child: Column(
                                   crossAxisAlignment: CrossAxisAlignment.start,
                                   children: [
                                     Text(friend['name'] ?? "User", style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
                                     Text(friend['in_game'] == true ? "Playing" : "Online", 
                                       style: TextStyle(color: friend['in_game'] == true ? Colors.orange : Colors.green, fontSize: 10)),
                                   ],
                                 ),
                               ),
                               // Button
                               ElevatedButton(
                                 onPressed: isInvited ? null : () => _inviteUser(id, friend['name']),
                                 style: ElevatedButton.styleFrom(
                                   backgroundColor: isInvited ? Colors.grey : Colors.green,
                                   padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
                                   minimumSize: const Size(60, 30),
                                   tapTargetSize: MaterialTapTargetSize.shrinkWrap,
                                 ),
                                 child: Text(isInvited ? "SENT" : "INVITE", style: const TextStyle(fontSize: 10, color: Colors.white)),
                               ),
                             ],
                           ),
                         );
                       },
                     ),
             )
           ],
         ),
       ),
    );
  }
}
