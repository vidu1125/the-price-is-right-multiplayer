import 'dart:async';
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
  StreamSubscription? _subscription;

  @override
  void initState() {
    super.initState();
    _loadProfile();
    _subscription = ServiceLocator.profileService.profileUpdates.listen((updatedProfile) {
        if (mounted) {
            setState(() => _profile = updatedProfile);
        }
    });
  }

  @override
  void dispose() {
    _subscription?.cancel();
    super.dispose();
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
    // Slightly reduced sizes as requested
    final double nameFontSize = (screenWidth * 0.018).clamp(16.0, 32.0);
    final double avatarSize = (screenWidth * 0.05).clamp(50.0, 80.0);

    return Container(
      // 1. Box hình viên thuốc (Pill shape) từ CSS .user-card
      decoration: LobbyTheme.userCardDecoration,
      padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12), // Increased padding
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
                child: _profile?.avatar != null && _profile!.avatar!.isNotEmpty
                    ? (_profile!.avatar!.startsWith("http")
                        ? Image.network(
                            _profile!.avatar!,
                            fit: BoxFit.cover,
                            errorBuilder: (c, e, s) => Image.asset('assets/images/default-mushroom.jpg', fit: BoxFit.cover),
                          )
                        : Image.asset(
                            _profile!.avatar!,
                            fit: BoxFit.cover,
                            errorBuilder: (c, e, s) => Image.asset('assets/images/default-mushroom.jpg', fit: BoxFit.cover),
                          ))
                    : Image.asset(
                        'assets/images/default-mushroom.jpg',
                        fit: BoxFit.cover,
                      ),
              ),
            ),
          ),
          
          const SizedBox(width: 16), // Increased spacing
          
          // 3. Player Name với Shadow đặc trưng từ CSS
          Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
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
              if (_profile != null && _profile!.accountId > 0)
                Text(
                  "ID: ${_profile!.accountId}",
                  style: TextStyle(
                    fontFamily: 'LuckiestGuy',
                    fontSize: nameFontSize * 0.8, // Significantly larger ID
                    color: Colors.blueGrey,
                    fontWeight: FontWeight.bold,
                  ),
                ),
            ],
          ),
        ],
      ),
    );
  }
}