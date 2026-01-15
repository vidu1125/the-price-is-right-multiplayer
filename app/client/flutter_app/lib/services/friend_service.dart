// lib/services/friend_service.dart
import 'dart:async';
import '../network/tcp_client.dart';
import '../network/dispatcher.dart';
import '../core/command.dart';
import '../core/protocol.dart';

enum FriendEventType {
  requestReceived,
  requestAccepted,
  requestRejected,
  friendAdded,
  friendRemoved,
  friendStatusChanged
}

class FriendEvent {
  final FriendEventType type;
  final dynamic data;
  FriendEvent(this.type, [this.data]);
}

class FriendService {
  final TcpClient client;
  final Dispatcher dispatcher;
  
  final _eventController = StreamController<FriendEvent>.broadcast();
  Stream<FriendEvent> get events => _eventController.stream;

  List<dynamic>? _cachedFriends; // In-memory cache

  FriendService(this.client, this.dispatcher) {
    _registerHandlers();
  }

  void _registerHandlers() {
    dispatcher.register(Command.ntfFriendRequest, (msg) {
        try {
          var json = Protocol.decodeJson(msg.payload);
          _eventController.add(FriendEvent(FriendEventType.requestReceived, json));
        } catch (_) {
          _eventController.add(FriendEvent(FriendEventType.requestReceived, null));
        }
    });

    dispatcher.register(Command.ntfFriendAccepted, (msg) {
        try {
          var json = Protocol.decodeJson(msg.payload);
          _cachedFriends = null; // Invalidate cache
          _eventController.add(FriendEvent(FriendEventType.requestAccepted, json));
        } catch (_) {}
    });

     dispatcher.register(Command.ntfFriendAdded, (msg) {
        try {
          var json = Protocol.decodeJson(msg.payload);
          _cachedFriends = null; // Invalidate cache
          _eventController.add(FriendEvent(FriendEventType.friendAdded, json));
        } catch (_) {}
    });   

    dispatcher.register(Command.ntfFriendRemoved, (msg) {
        try {
          var json = Protocol.decodeJson(msg.payload);
          _cachedFriends = null; // Invalidate cache
          _eventController.add(FriendEvent(FriendEventType.friendRemoved, json));
        } catch (_) {}
    });
    
    // Status changes - Update cache in-place if possible, or invalidate
    dispatcher.register(Command.ntfFriendStatus, (msg) {
       try {
          var json = Protocol.decodeJson(msg.payload);
          // Optional: Update status in cache without full refetch
          if (_cachedFriends != null && json['friend_id'] != null) {
              final id = json['friend_id'];
              final idx = _cachedFriends!.indexWhere((f) => f['id'] == id);
              if (idx != -1) {
                  _cachedFriends![idx]['online'] = (json['status'] != 'offline');
                  _cachedFriends![idx]['in_game'] = (json['status'] == 'playing');
              }
          }
          _eventController.add(FriendEvent(FriendEventType.friendStatusChanged, json));
        } catch (_) {}
    });
  }

  /// Search for users by name or email (or ID)
  Future<Map<String, dynamic>> searchUser(String query) async {
    try {
      final response = await client.request(
        Command.searchUser,
        payload: Protocol.jsonPayload({"query": query}),
      );

      final data = Protocol.decodeJson(response.payload);
      
      if (data["success"] == true) {
        return {
          "success": true,
          "results": data["users"] ?? [],
          "count": data["count"] ?? 0
        };
      }
      return {"success": false, "error": data["error"] ?? "Search failed"};
    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  /// Send a friend request
  Future<Map<String, dynamic>> sendFriendRequest(int friendId) async {
    try {
      final response = await client.request(
        Command.friendAdd, // 0x0501
        payload: Protocol.jsonPayload({"friend_id": friendId}),
      );

      final data = Protocol.decodeJson(response.payload);
      
      if (response.command == Command.resFriendAdded || data["success"] == true) {
         return {"success": true, "message": "Friend request sent"};
      }

      return {"success": false, "error": data["error"] ?? "Failed to add friend"};
    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  /// Get list of friends with caching
  Future<Map<String, dynamic>> getFriendList({bool forceRefresh = false}) async {
    // Return cache if available and not forcing refresh
    if (!forceRefresh && _cachedFriends != null) {
        return {"success": true, "friends": _cachedFriends!};
    }

    try {
      final response = await client.request(
        Command.friendList,
        payload: Protocol.jsonPayload({}),
      );

      final data = Protocol.decodeJson(response.payload);
      if (data["success"] == true) {
        _cachedFriends = data["friends"] ?? [];
        return {"success": true, "friends": _cachedFriends!};
      }
      return {"success": false, "error": data["error"] ?? "Failed to fetch friends"};
    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  /// Get list of friend requests
  Future<Map<String, dynamic>> getFriendRequests() async {
    try {
      final response = await client.request(
        Command.friendRequests, // 0x0509
        payload: Protocol.jsonPayload({}),
      );
      
      // Should be resFriendRequests (0x00EA)
      final data = Protocol.decodeJson(response.payload);
      if (data["success"] == true) {
          return {"success": true, "requests": data["requests"] ?? [], "count": data["count"] ?? 0};
      }
      return {"success": false, "error": data["error"] ?? "Failed to fetch requests"};
    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  /// Accept a friend request
  Future<Map<String, dynamic>> acceptRequest(int requestId) async {
    try {
      final response = await client.request(
        Command.friendAccept, // 0x0505
        payload: Protocol.jsonPayload({"request_id": requestId}),
      );
      
      final data = Protocol.decodeJson(response.payload);
      if (data["success"] == true) {
          return {"success": true, "message": "Request accepted"};
      }
       return {"success": false, "error": data["error"] ?? "Failed to accept"};
    } catch (e) {
       return {"success": false, "error": e.toString()};
    }
  }

  /// Reject a friend request
  Future<Map<String, dynamic>> rejectRequest(int requestId) async {
    try {
       final response = await client.request(
        Command.friendReject, // 0x0506
        payload: Protocol.jsonPayload({"request_id": requestId}),
      );
      
      final data = Protocol.decodeJson(response.payload);
      if (data["success"] == true) {
          return {"success": true, "message": "Request rejected"};
      }
      return {"success": false, "error": data["error"] ?? "Failed to reject"};
    } catch (e) {
       return {"success": false, "error": e.toString()};
    }
  }

  /// Remove a friend
  Future<Map<String, dynamic>> removeFriend(int friendId) async {
    try {
       final response = await client.request(
        Command.friendRemove, // 0x0507
        payload: Protocol.jsonPayload({"friend_id": friendId}),
       );
       
       final data = Protocol.decodeJson(response.payload);
       if (data["success"] == true) {
           return {"success": true, "message": "Friend removed"};
       }
       return {"success": false, "error": data["error"] ?? "Failed to remove friend"};
    } catch (e) {
       return {"success": false, "error": e.toString()};
    }
  }

  void dispose() {
    _eventController.close();
  }
}
