import '../models/user_profile.dart';

// Giả lập service cập nhật profile
class ProfileService {
  Future<Map<String, dynamic>> updateProfile(UserProfile profile) async {
    await Future.delayed(const Duration(seconds: 1)); // Giả lập độ trễ mạng
    return {"success": true};
  }
}
