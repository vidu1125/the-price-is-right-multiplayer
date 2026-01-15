// lib/ui/widgets/add_friend_dialog.dart
import 'dart:async';
import 'package:flutter/material.dart';
import '../../services/service_locator.dart';
import '../../services/friend_service.dart';
import '../theme/lobby_theme.dart';

class AddFriendDialog extends StatefulWidget {
  final int initialIndex;
  const AddFriendDialog({super.key, this.initialIndex = 0});

  @override
  State<AddFriendDialog> createState() => _AddFriendDialogState();
}

class _AddFriendDialogState extends State<AddFriendDialog> with SingleTickerProviderStateMixin {
  late TabController _tabController;
  
  // Data
  List<dynamic> _friends = [];
  List<dynamic> _requests = [];
  List<dynamic> _searchResults = [];
  
  // Search
  final TextEditingController _searchController = TextEditingController();
  final Set<int> _sentRequests = {};

  // Status
  bool _isLoadingFriends = false;
  bool _isLoadingRequests = false;
  bool _isSearching = false;
  
  StreamSubscription? _friendSub;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 3, vsync: this, initialIndex: widget.initialIndex);
    _tabController.addListener(() {
        if (_tabController.index == 1) { // 1 is Requests tab
             _fetchRequests();
        } else if (_tabController.index == 0) {
             _fetchFriends();
        }
    });

    _fetchFriends();
    _fetchRequests();
    
    // Listen for real-time updates
    _friendSub = ServiceLocator.friendService.events.listen((event) {
        if (!mounted) return;
        
        if (event.type == FriendEventType.requestReceived) {
             // "Push" notification directly to list
             if (event.data != null) {
                 try {
                     final newReq = Map<String, dynamic>.from(event.data);
                     // Map keys from NTF to List item format
                     if (newReq.containsKey('request_id')) {
                         newReq['id'] = newReq['request_id'];
                     }
                     
                     // Check duplicates
                     final exists = _requests.any((r) => r['id'] == newReq['id']);
                     if (!exists) {
                         setState(() {
                             _requests.insert(0, newReq);
                             // Switch to Requests tab if we received one
                             // _tabController.animateTo(1); 
                         });
                         ScaffoldMessenger.of(context).showSnackBar(
                           SnackBar(content: Text("New friend request from ${newReq['sender_name'] ?? 'someone'}!"))
                         );
                     }
                 } catch (e) {
                     print("Error pushing request: $e");
                 }
             }
             // Fetch to ensure consistency
             _fetchRequests();
        } else if (event.type == FriendEventType.requestAccepted ||
            event.type == FriendEventType.requestRejected) {
             _fetchRequests();
             _fetchFriends();
        }
        
        if (event.type == FriendEventType.friendAdded || event.type == FriendEventType.friendRemoved) {
             _fetchFriends();
        }
    });
  }

  @override
  void dispose() {
    _tabController.dispose();
    _searchController.dispose();
    _friendSub?.cancel();
    super.dispose();
  }

  Future<void> _fetchFriends() async {
    setState(() => _isLoadingFriends = true);
    final res = await ServiceLocator.friendService.getFriendList();
    if (mounted) {
       setState(() {
         _isLoadingFriends = false;
         if (res["success"] == true) {
             _friends = res["friends"] ?? [];
         }
       });
    }
  }

  Future<void> _fetchRequests() async {
    setState(() => _isLoadingRequests = true);
    final res = await ServiceLocator.friendService.getFriendRequests();
    print("[DEBUG] _fetchRequests response: $res");
    if (mounted) {
       setState(() {
         _isLoadingRequests = false;
         if (res["success"] == true) {
             _requests = res["requests"] ?? [];
         }
       });
    }
  }

  Future<void> _handleSearch() async {
    final query = _searchController.text.trim();
    if (query.isEmpty) return;

    setState(() {
       _isSearching = true;
       _searchResults = [];
    });

    final res = await ServiceLocator.friendService.searchUser(query);
    if (mounted) {
       setState(() {
          _isSearching = false;
          if (res["success"] == true) {
             _searchResults = res["results"] ?? [];
          } else {
             _searchResults = [];
             ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(res["error"] ?? "Search failed")));
          }
       });
    }
  }

  Future<void> _sendRequest(int userId) async {
      if (_sentRequests.contains(userId)) return;
      final res = await ServiceLocator.friendService.sendFriendRequest(userId);
      if (mounted) {
          if (res["success"] == true) {
              setState(() => _sentRequests.add(userId));
              ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text("Request sent!")));
          } else {
              ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(res["error"] ?? "Failed")));
          }
      }
  }

  Future<void> _acceptRequest(int requestId) async {
     final res = await ServiceLocator.friendService.acceptRequest(requestId);
     if (res["success"] == true) {
         _fetchRequests();
         _fetchFriends();
     } else {
         if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(res["error"] ?? "Failed")));
     }
  }

  Future<void> _rejectRequest(int requestId) async {
     final res = await ServiceLocator.friendService.rejectRequest(requestId);
     if (res["success"] == true) {
         _fetchRequests();
     } else {
         if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(res["error"] ?? "Failed")));
     }
  }

  Future<void> _removeFriend(int friendId) async {
     final confirm = await showDialog<bool>(
        context: context,
        builder: (_) => AlertDialog(
            backgroundColor: LobbyTheme.primaryDark,
            shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(16),
                side: const BorderSide(color: LobbyTheme.yellowGame, width: 2)
            ),
            title: const Text("REMOVE FRIEND", style: TextStyle(color: LobbyTheme.yellowGame, fontFamily: 'LuckiestGuy')), 
            content: const Text("Are you sure you want to remove this friend?", style: TextStyle(color: Colors.white)),
            actions: [
                TextButton(
                    onPressed: ()=>Navigator.pop(context,false), 
                    child: const Text("Cancel", style: TextStyle(color: Colors.white70))
                ),
                TextButton(
                    onPressed: ()=>Navigator.pop(context,true), 
                    child: const Text("Remove", style: TextStyle(color: Colors.red, fontWeight: FontWeight.bold))
                ),
            ]
        )
     );
     
     if (confirm == true) {
         // Optimistic update: Remove locally first
         final userToRemove = _friends.firstWhere((f) => f['id'] == friendId, orElse: () => null);
         setState(() {
             _friends.removeWhere((f) => f['id'] == friendId);
         });

         final res = await ServiceLocator.friendService.removeFriend(friendId);
         
         if (res["success"] != true) {
             // Failed, restore list and show error
             if (mounted) {
                 ScaffoldMessenger.of(context).showSnackBar(
                     SnackBar(content: Text(res["error"] ?? "Failed to remove friend"))
                 );
                 // Restore or just fetch
                 _fetchFriends();
             }
         }
     }
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
            width: 500,
            height: 600,
            padding: const EdgeInsets.all(20),
            child: Column(
                children: [
                    // Header with Tabs
                    Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                            const Text("FRIENDS", style: TextStyle(fontFamily: 'LuckiestGuy', fontSize: 24, color: LobbyTheme.yellowGame)),
                            IconButton(icon: const Icon(Icons.close, color: Colors.white), onPressed: () => Navigator.pop(context))
                        ]
                    ),
                    const SizedBox(height: 10),
                    TabBar(
                        controller: _tabController,
                        indicatorColor: LobbyTheme.yellowGame,
                        labelColor: LobbyTheme.yellowGame,
                        unselectedLabelColor: Colors.white54,
                        tabs: [
                            const Tab(icon: Icon(Icons.people), text: "My Friends"),
                            Tab(child: Row(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                    const Icon(Icons.notifications),
                                    const SizedBox(width: 5),
                                    const Text("Requests"),
                                    if (_requests.isNotEmpty) ...[
                                        const SizedBox(width: 5),
                                        Container(
                                            padding: const EdgeInsets.all(4),
                                            decoration: const BoxDecoration(color: Colors.red, shape: BoxShape.circle),
                                            child: Text("${_requests.length}", style: const TextStyle(fontSize: 10, color: Colors.white))
                                        )
                                    ]
                                ]
                            )),
                            const Tab(icon: Icon(Icons.person_add), text: "Add Friend"),
                        ]
                    ),
                    const SizedBox(height: 10),
                    Expanded(
                        child: TabBarView(
                            controller: _tabController,
                            children: [
                                _buildFriendsTab(),
                                _buildRequestsTab(),
                                _buildSearchTab(),
                            ]
                        )
                    )
                ]
            )
        )
    );
  }

  Widget _buildFriendsTab() {
      if (_isLoadingFriends) return const Center(child: CircularProgressIndicator());
      if (_friends.isEmpty) return const Center(child: Text("No friends yet.", style: TextStyle(color: Colors.white54)));
      
      return ListView.builder(
          itemCount: _friends.length,
          itemBuilder: (context, index) {
              final f = _friends[index];
              final bool isOnline = f["online"] == true;
              return Container(
                  margin: const EdgeInsets.only(bottom: 8),
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(color: Colors.white10, borderRadius: BorderRadius.circular(10)),
                  child: Row(
                      children: [
                          _buildAvatar(f["avatar"]),
                          const SizedBox(width: 10),
                          Expanded(
                              child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                      Text(f["name"] ?? "User", style: LobbyTheme.gameFont(fontSize: 16)),
                                      Text(isOnline ? "Online" : "Offline", style: TextStyle(color: isOnline ? Colors.green : Colors.grey, fontSize: 12))
                                  ]
                              )
                          ),
                          IconButton(
                              icon: const Icon(Icons.person_remove, color: Colors.redAccent),
                              onPressed: () => _removeFriend(f["id"]),
                              tooltip: "Remove friend",
                          )
                      ]
                  )
              );
          }
      );
  }

  Widget _buildRequestsTab() {
      if (_isLoadingRequests) return const Center(child: CircularProgressIndicator());
      if (_requests.isEmpty) return const Center(child: Text("No pending requests.", style: TextStyle(color: Colors.white54)));
      
      return ListView.builder(
          itemCount: _requests.length,
          itemBuilder: (context, index) {
              final req = _requests[index];
              return Container(
                  margin: const EdgeInsets.only(bottom: 8),
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(color: Colors.white10, borderRadius: BorderRadius.circular(10)),
                  child: Row(
                      children: [
                          _buildAvatar(req["sender_avatar"]),
                          const SizedBox(width: 10),
                          Expanded(
                              child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                      Text(req["sender_name"] ?? "User", style: LobbyTheme.gameFont(fontSize: 16)),
                                      Text("Sent you a request", style: const TextStyle(color: Colors.white54, fontSize: 12))
                                  ]
                              )
                          ),
                          IconButton(
                              icon: const Icon(Icons.check_circle, color: Colors.green),
                              onPressed: () => _acceptRequest(req["id"]),
                              tooltip: "Accept",
                          ),
                          IconButton(
                              icon: const Icon(Icons.cancel, color: Colors.red),
                              onPressed: () => _rejectRequest(req["id"]),
                              tooltip: "Reject",
                          )
                      ]
                  )
              );
          }
      );
  }

  Widget _buildSearchTab() {
      return Column(
          children: [
              Row(
                  children: [
                      Expanded(
                          child: TextField(
                              controller: _searchController,
                              style: const TextStyle(color: Colors.white),
                              decoration: InputDecoration(
                                  hintText: "Search User",
                                  hintStyle: const TextStyle(color: Colors.white54),
                                  filled: true,
                                  fillColor: Colors.black26,
                                  border: OutlineInputBorder(borderRadius: BorderRadius.circular(8)),
                                  suffixIcon: IconButton(icon: const Icon(Icons.search, color: LobbyTheme.yellowGame), onPressed: _handleSearch)
                              ),
                              onSubmitted: (_) => _handleSearch(),
                          )
                      ),
                  ]
              ),
              const SizedBox(height: 10),
              if (_isSearching) const CircularProgressIndicator(),
              if (!_isSearching && _searchResults.isNotEmpty)
                  Expanded(
                      child: ListView.builder(
                          itemCount: _searchResults.length,
                          itemBuilder: (context, index) {
                              final u = _searchResults[index];
                              final isFriend = u["is_friend"] == true;
                              final id = u["id"];
                              final isSent = _sentRequests.contains(id);

                              return Container(
                                  margin: const EdgeInsets.only(bottom: 8),
                                  padding: const EdgeInsets.all(10),
                                  decoration: BoxDecoration(color: Colors.white10, borderRadius: BorderRadius.circular(10)),
                                  child: Row(
                                      children: [
                                          _buildAvatar(u["avatar"]),
                                          const SizedBox(width: 10),
                                          Expanded(
                                              child: Column(
                                                  crossAxisAlignment: CrossAxisAlignment.start,
                                                  children: [
                                                      Text(u["name"] ?? "User", style: LobbyTheme.gameFont(fontSize: 16)),
                                                      Text("#$id", style: const TextStyle(color: Colors.white54, fontSize: 12))
                                                  ]
                                              )
                                          ),
                                          if (isFriend)
                                              const Chip(label: Text("Friend"), backgroundColor: Colors.green)
                                          else
                                              ElevatedButton(
                                                  onPressed: isSent ? null : () => _sendRequest(id),
                                                  style: ElevatedButton.styleFrom(backgroundColor: isSent ? Colors.grey : LobbyTheme.yellowGame),
                                                  child: Text(isSent ? "Sent" : "Add", style: const TextStyle(color: Colors.black))
                                              )
                                      ]
                                  )
                              );
                          }
                      )
                  )
              else if (!_isSearching && _searchController.text.isNotEmpty)
                  const Text("No results found.", style: TextStyle(color: Colors.white54))
          ]
      );
  }

  Widget _buildAvatar(dynamic avatar) {
      return Container(
          width: 40, height: 40,
          decoration: BoxDecoration(
              shape: BoxShape.circle,
              border: Border.all(color: LobbyTheme.yellowGame),
              image: DecorationImage(
                  fit: BoxFit.cover,
                  image: (avatar != null && avatar.toString().startsWith("http")) 
                     ? NetworkImage(avatar) 
                     : const AssetImage("assets/images/default-mushroom.jpg") as ImageProvider
              )
          )
      );
  }
}
