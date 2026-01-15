import 'dart:convert';
import 'package:shared_preferences/shared_preferences.dart';
import '../network/tcp_client.dart';
import '../core/command.dart';
import '../core/protocol.dart';
import '../core/config.dart';

class AuthService {
  final TcpClient client;

  AuthService(this.client);

  // --- Core Methods ---

  Future<Map<String, dynamic>> register({
    required String email,
    required String password,
    required String name,
  }) async {
    try {
      if (client.status != ConnectionStatus.connected) {
         await client.connect(AppConfig.serverHost, AppConfig.serverPort);
      }

      final response = await client.request(
        Command.registerReq,
        payload: Protocol.jsonPayload({
          "email": email.trim().toLowerCase(),
          "password": password,
          "name": name,
        }),
      );

      final data = Protocol.decodeJson(response.payload);
      if (response.command == Command.resSuccess || response.command == Command.resLoginOk) {
        await persistAuth(data);
        return {"success": true, "data": data};
      } else {
        return {"success": false, "error": data["error"] ?? "Registration failed"};
      }
    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  Future<Map<String, dynamic>> login(String email, String password) async {
    try {
      if (client.status != ConnectionStatus.connected) {
         await client.connect(AppConfig.serverHost, AppConfig.serverPort);
      }

      final response = await client.request(
        Command.loginReq,
        payload: Protocol.jsonPayload({
          "email": email.trim().toLowerCase(),
          "password": password,
        }),
      );

      if (response.command == Command.resLoginOk || response.command == Command.resSuccess) {
         final data = Protocol.decodeJson(response.payload);
         await persistAuth(data);
         return {"success": true, "data": data};
      } else {
         // Error response might be plain text
         String errorMsg;
         try {
           final data = Protocol.decodeJson(response.payload);
           errorMsg = data["error"] ?? "Login failed (error code 0x${response.command.toRadixString(16)})";
         } catch (_) {
           // Fallback for plain text payload
           errorMsg = String.fromCharCodes(response.payload);
         }
         return {"success": false, "error": errorMsg};
      }
    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  Future<Map<String, dynamic>> reconnect(String sessionId) async {
    try {
      final prefs = await SharedPreferences.getInstance();
      final accountIdStr = prefs.getString("account_id");
      
      final Map<String, dynamic> payload = {
        "session_id": sessionId,
      };
      
      if (accountIdStr != null) {
        payload["account_id"] = int.tryParse(accountIdStr);
      }

      final response = await client.request(
        Command.reconnect,
        payload: Protocol.jsonPayload(payload),
      );

      final data = Protocol.decodeJson(response.payload);
      if (response.command == Command.resLoginOk || response.command == Command.resSuccess) {
        await persistAuth(data);
        
        return {"success": true, "data": data};
      } else {
        await clearAuth();
        return {"success": false, "error": data["error"] ?? "Reconnect failed"};
      }
    } catch (e) {
      return {"success": false, "error": e.toString()};
    }
  }

  Future<Map<String, dynamic>> logout() async {
    final prefs = await SharedPreferences.getInstance();
    final sessionId = prefs.getString("session_id");

    if (sessionId == null) {
      await clearAuth();
      client.disconnect();
      return {"success": true, "message": "Already logged out"};
    }

    try {
      final response = await client.request(
        Command.logoutReq,
        payload: Protocol.jsonPayload({"session_id": sessionId}),
      );

      final data = Protocol.decodeJson(response.payload);
      if (response.command == Command.resSuccess || response.command == Command.resLoginOk) {
        await clearAuth();
        client.disconnect();
        return {"success": true, "data": data};
      } else {
        await clearAuth(); // Clear anyway on error?
        client.disconnect();
        return {"success": false, "error": data["error"] ?? "Logout failed"};
      }
    } catch (e) {
      print("Logout request failed: $e");
      // Force clear
      await clearAuth();
      client.disconnect();
      return {"success": false, "error": e.toString()};
    }
  }

  // --- Persistence Methods ---

  Future<void> persistAuth(Map<String, dynamic> data) async {
    final prefs = await SharedPreferences.getInstance();
    if (data.containsKey("account_id")) {
      await prefs.setString("account_id", data["account_id"].toString());
    }
    if (data.containsKey("session_id")) {
      await prefs.setString("session_id", data["session_id"].toString());
    }
    if (data.containsKey("profile")) {
      await prefs.setString("profile", jsonEncode(data["profile"]));
      if (data["profile"].containsKey("email")) {
        await prefs.setString("email", data["profile"]["email"]);
      }
    }
    
    // Save current_room if user was in a room
    if (data.containsKey("current_room") && data["current_room"] != null) {
      await prefs.setString("pending_room_restore", jsonEncode(data["current_room"]));
      print("💾 Saved pending room restore: ${data["current_room"]["room_name"]}");
    }
  }

  Future<Map<String, dynamic>> getAuthState() async {
    final prefs = await SharedPreferences.getInstance();
    final profileStr = prefs.getString("profile");
    return {
      "accountId": prefs.getString("account_id"),
      "sessionId": prefs.getString("session_id"),
      "email": prefs.getString("email"),
      "profile": profileStr != null ? jsonDecode(profileStr) : {},
    };
  }

  Future<void> clearAuth() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove("account_id");
    await prefs.remove("session_id");
    await prefs.remove("email");
    await prefs.remove("profile");
  }

  Future<bool> isAuthenticated() async {
    final state = await getAuthState();
    return state["accountId"] != null && state["sessionId"] != null;
  }
}
