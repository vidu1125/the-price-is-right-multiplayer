import 'dart:async';
import 'dart:convert';
import '../network/tcp_client.dart';
import '../core/command.dart';
import '../core/protocol.dart';
import '../models/user_profile.dart';
import 'package:shared_preferences/shared_preferences.dart';

class ProfileService {
  final TcpClient client;

  ProfileService(this.client);

  Future<UserProfile?> getProfile({bool forceRefresh = false}) async {
    // 1. Try to load from cache if not forcing refresh
    if (!forceRefresh) {
      final cached = await _loadFromCache();
      if (cached != null) {
        return cached;
      }
    }

    try {
      // 2. Request from server
      final response = await client.request(
        Command.cmdGetProfile,
        payload: Protocol.jsonPayload({}), // Empty payload
      );

      final data = Protocol.decodeJson(response.payload);

      if (response.command == Command.resProfileFound) {
        if (data["success"] == true && data["profile"] != null) {
          final profile = UserProfile.fromJson(data["profile"]);
          await _saveToCache(data["profile"]);
          return profile;
        }
      }
      return null;
    } catch (e) {
      print("Error fetching profile: $e");
      // Fallback to cache on error
      return await _loadFromCache();
    }
  }

  Future<Map<String, dynamic>> updateProfile(UserProfile profile) async {
    try {
      final response = await client.request(
        Command.cmdUpdateProfile,
        payload: Protocol.jsonPayload({
          "name": profile.name,
          "avatar": profile.avatar,
          "bio": profile.bio,
        }),
      );

      final data = Protocol.decodeJson(response.payload);
      if (response.command == Command.resProfileUpdated || response.command == Command.resProfileFound) {
         if (data["success"] == true && data["profile"] != null) {
           await _saveToCache(data["profile"]);
           return {"success": true, "data": data};
         }
      }
      return {"success": false, "error": data["error"] ?? "Update failed"};

    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  Future<void> _saveToCache(Map<String, dynamic> json) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString("profile", jsonEncode(json));
  }

  Future<UserProfile?> _loadFromCache() async {
    final prefs = await SharedPreferences.getInstance();
    final str = prefs.getString("profile");
    if (str != null) {
      try {
        return UserProfile.fromJson(jsonDecode(str));
      } catch (e) {
        print("Error parsing cached profile: $e");
      }
    }
    return null;
  }
}
