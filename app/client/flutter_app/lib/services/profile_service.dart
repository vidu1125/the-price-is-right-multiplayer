import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
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

  final _profileUpdateController = StreamController<UserProfile>.broadcast();
  Stream<UserProfile> get profileUpdates => _profileUpdateController.stream;

  Future<Map<String, dynamic>> updateProfile(UserProfile profile) async {
    try {
      print("[ProfileService] Sending update cmd: ${Command.cmdUpdateProfile}");
      final response = await client.request(
        Command.cmdUpdateProfile,
        payload: Protocol.jsonPayload({
          "name": profile.name,
          "avatar": profile.avatar,
          "bio": profile.bio,
        }),
      );
      print("[ProfileService] Got response: ${response.command}");

      final data = Protocol.decodeJson(response.payload);
      if (response.command == Command.resProfileUpdated || response.command == Command.resProfileFound) {
         if (data["success"] == true && data["profile"] != null) {
           await _saveToCache(data["profile"]);
           final updatedProfile = UserProfile.fromJson(data["profile"]);
           _profileUpdateController.add(updatedProfile);
           return {"success": true, "data": data};
         }
      }
      return {"success": false, "error": data["error"] ?? "Update failed"};

    } catch (e) {
      print("[ProfileService] Error: $e");
      return {"success": false, "error": e.toString()};
    }
  }

  Future<Map<String, dynamic>> changePassword(String oldPassword, String newPassword) async {
    try {
      // Build binary payload: struct { char old[64]; char new[64]; }
      final buffer = Uint8List(128);
      
      final oldBytes = utf8.encode(oldPassword);
      final newBytes = utf8.encode(newPassword);

      // Copy old password (max 64 bytes)
      for (int i = 0; i < oldBytes.length && i < 64; i++) {
        buffer[i] = oldBytes[i];
      }
      
      // Copy new password (max 64 bytes) at offset 64
      for (int i = 0; i < newBytes.length && i < 64; i++) {
        buffer[64 + i] = newBytes[i];
      }

      final response = await client.request(
        Command.cmdChangePassword,
        payload: buffer,
      );

      // Response is JSON
      final data = Protocol.decodeJson(response.payload);
      if (response.command == Command.resPasswordChanged) {
        if (data["success"] == true) {
          return {"success": true, "message": "Password changed successfully"};
        }
      }
      return {"success": false, "error": data["error"] ?? "Failed to change password"};
    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  void dispose() {
    _profileUpdateController.close();
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
