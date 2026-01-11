import 'package:flutter/material.dart';
import '../../services/service_locator.dart';
import '../../models/user_profile.dart';
import '../theme/lobby_theme.dart';

class UserCard extends StatefulWidget {
  const UserCard({super.key});

  @override
  State<UserCard> createState() => _UserCardState();
}

class _UserCardState extends State<UserCard> {
  UserProfile? _profile;
  bool _isLoading = true;

  @override
  void initState() {
    super.initState();
    _loadProfile();
  }

  Future<void> _loadProfile() async {
    if (!await ServiceLocator.authService.isAuthenticated()) return;
    
    final profile = await ServiceLocator.profileService.getProfile();
    if (mounted) {
      setState(() {
        _profile = profile;
        _isLoading = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    final name = _profile?.name ?? "Guest";
    // Check if avatar string is a URL or empty, otherwise use default
    // For now simplifying to default asset, can be improved later
    
    if (_isLoading) {
      return Container(
         decoration: LobbyTheme.userCardDecoration,
         padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
         child: const SizedBox(
           width: 20, 
           height: 20, 
           child: CircularProgressIndicator(strokeWidth: 2)
         ),
      );
    }

    final screenWidth = MediaQuery.of(context).size.width;
    final double nameFontSize = (screenWidth * 0.015).clamp(14.0, 24.0);
    final double avatarSize = (screenWidth * 0.04).clamp(40.0, 60.0);

    return Container(
      // 1. Box hình viên thuốc (Pill shape) từ CSS .user-card
      decoration: LobbyTheme.userCardDecoration,
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          // 2. Avatar với viền kép (Double border effect)
          GestureDetector(
            onTap: () {
               print("Open profile modal: ${_profile?.toJson()}");
               _loadProfile(); // Tap to refresh for now
            },
            child: Container(
              width: avatarSize,
              height: avatarSize,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: const Color(0xFFF8F8F8), // Nền trắng nhẹ cho avatar
                border: Border.all(color: LobbyTheme.primaryDark, width: 4),
              ),
              child: ClipOval(
                child: Image.asset(
                  'assets/images/default-mushroom.jpg',
                  fit: BoxFit.cover,
                  // Dự phòng nếu không tìm thấy ảnh
                  errorBuilder: (context, error, stackTrace) => Icon(
                    Icons.person,
                    color: LobbyTheme.primaryDark,
                    size: avatarSize * 0.6,
                  ),
                ),
              ),
            ),
          ),
          
          const SizedBox(width: 12),
          
          // 3. Player Name với Shadow đặc trưng từ CSS
          Text(
            name,
            style: TextStyle(
              fontFamily: 'LuckiestGuy',
              fontSize: nameFontSize,
              color: LobbyTheme.primaryDark,
              // Hiệu ứng text-shadow: 1px 1px 0 #fff trong CSS
              shadows: const [
                Shadow(offset: Offset(1, 1), color: Colors.white),
                Shadow(offset: Offset(-1, -1), color: Colors.white),
              ],
            ),
          ),
        ],
      ),
    );
  }
}