import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import '../theme/tutorial_theme.dart';
import '../theme/lobby_theme.dart';
import '../../services/service_locator.dart';
import '../../models/user_profile.dart';

class PlayerSettingsPage extends StatefulWidget {
  const PlayerSettingsPage({super.key});

  @override
  State<PlayerSettingsPage> createState() => _PlayerSettingsPageState();
}

class _PlayerSettingsPageState extends State<PlayerSettingsPage> {
  // State variables tương ứng với useState trong React
  final TextEditingController _nameController = TextEditingController(text: "Player 1");
  final TextEditingController _avatarController = TextEditingController();
  final TextEditingController _bioController = TextEditingController();
  
  bool _isSaving = false;
  String _msg = "";
  String _error = "";
  final int _maxName = 10;

  final List<String> _presetAvatars = [
    "assets/images/default-mushroom.jpg",
    "assets/images/duck.jpg", 
    "assets/images/frog.jpg",
    "assets/images/sth.jpg",
    "assets/images/crown.png"
  ];

  @override
  void initState() {
    super.initState();
    // Reset message khi người dùng thay đổi input
    _nameController.addListener(() => setState(() => _msg = ""));
    _bioController.addListener(() => setState(() => _msg = ""));
    
    // Load current profile
    _loadCurrentProfile();
  }

  Future<void> _loadCurrentProfile() async {
    final profile = await ServiceLocator.profileService.getProfile();
    if (profile != null) {
      setState(() {
        _nameController.text = profile.name ?? "";
        _avatarController.text = profile.avatar ?? _presetAvatars[0];
        _bioController.text = profile.bio ?? "";
      });
    } else {
       if (_avatarController.text.isEmpty) {
          _avatarController.text = _presetAvatars[0];
       }
    }
  }

  Future<void> _onSubmit() async {
    print("[Settings] Submit button pressed");
    setState(() {
      _isSaving = true;
      _msg = "";
      _error = "";
    });

    final name = _nameController.text.trim();
    final avatar = _avatarController.text.trim();
    print("[Settings] Name: $name, Avatar: $avatar");

    // Validation logic
    if (name.isEmpty) {
      setState(() {
        _error = "Display name cannot be empty";
        _isSaving = false;
      });
      return;
    }

    try {
      print("[Settings] Calling updateProfile...");
      final res = await ServiceLocator.profileService.updateProfile(
        UserProfile(name: name, avatar: avatar, bio: _bioController.text)
      );
      print("[Settings] Result: $res");
      
      if (res['success'] == true) {
        setState(() => _msg = "Profile updated successfully!");
      } else {
        setState(() => _msg = res['error'] ?? "An error occurred");
      }
    } catch (err) {
      print("[Settings] Exception: $err");
      setState(() => _msg = "Could not update profile: $err");
    } finally {
      setState(() => _isSaving = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFE3F2FD),
      body: Stack(
        children: [
          Positioned.fill(
            child: Image.asset("assets/images/lobby-bg.png", fit: BoxFit.cover),
          ),
          Positioned.fill(child: Container(color: Colors.black.withOpacity(0.5))),
          Center(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(40),
              child: Container(
                constraints: const BoxConstraints(maxWidth: 1200),
                decoration: BoxDecoration(
                  color: const Color(0xB31F2A44),
                  borderRadius: BorderRadius.circular(24),
                  border: Border.all(color: const Color(0xFF1F2A44), width: 4),
                  boxShadow: [
                    BoxShadow(
                      color: Colors.black.withOpacity(0.18),
                      blurRadius: 28,
                      offset: const Offset(0, 10),
                    )
                  ],
                ),
                child: Column(
                  children: [
                    _buildHeader(),
                    Padding(
                      padding: const EdgeInsets.all(24),
                      child: LayoutBuilder(
                        builder: (context, constraints) {
                          // Responsive Grid: 2 cột trên màn hình rộng, 1 cột trên mobile
                          if (constraints.maxWidth > 700) {
                            return Row(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Expanded(flex: 11, child: _buildForm()),
                                const SizedBox(width: 40),
                                Expanded(flex: 9, child: _buildPreview()),
                              ],
                            );
                          } else {
                            return Column(
                              children: [
                                _buildPreview(),
                                const SizedBox(height: 32),
                                _buildForm(),
                              ],
                            );
                          }
                        },
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
  Widget _buildForm() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _buildLabel("Display Name"),
        TextField(
          controller: _nameController,
          maxLength: _maxName,
          onChanged: (val) => setState(() {}),
          decoration: _inputDecoration("Enter your name"),
        ),
        const SizedBox(height: 14),
        _buildLabel("Choose Avatar"),
        _buildAvatarPresets(),
        const SizedBox(height: 14),
        _buildLabel("Bio"),
        TextField(
          controller: _bioController,
          maxLines: 4,
          onChanged: (val) => setState(() {}), // Update preview
          decoration: _inputDecoration("Brief description about you"),
        ),
        if (_error.isNotEmpty) Padding(
          padding: const EdgeInsets.only(top: 8),
          child: Text(_error, style: const TextStyle(color: Color(0xFFC62828), fontWeight: FontWeight.bold)),
        ),
        if (_msg.isNotEmpty) Padding(
          padding: const EdgeInsets.only(top: 8),
          child: Text(_msg, style: const TextStyle(color: Color(0xFF2B8A3E), fontWeight: FontWeight.bold)),
        ),
        const SizedBox(height: 20),
        SizedBox(
          width: double.infinity,
          child: OutlinedButton(
            onPressed: _showChangePasswordDialog,
            style: OutlinedButton.styleFrom(
              side: const BorderSide(color: Color(0xFF29B6F6), width: 2),
              padding: const EdgeInsets.symmetric(vertical: 24),
              foregroundColor: const Color(0xFF29B6F6),
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
            ),
            child: const Text("CHANGE PASSWORD", style: TextStyle(fontWeight: FontWeight.bold, fontSize: 20)),
          ),
        ),
        const SizedBox(height: 12),
        SizedBox(
          width: double.infinity,
          child: ElevatedButton(
            onPressed: _isSaving ? null : _onSubmit,
            style: ElevatedButton.styleFrom(
              backgroundColor: const Color(0xFF29B6F6),
              padding: const EdgeInsets.symmetric(vertical: 24),
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(14),
                side: const BorderSide(color: Color(0xFF1F2A44), width: 2),
              ),
              elevation: 6,
            ),
            child: Text(
              _isSaving ? "SAVING..." : "SAVE CHANGES",
              style: GoogleFonts.luckiestGuy(
                fontSize: 28,
                color: Colors.white,
                shadows: _textOutlineShadows(),
              ),
            ),
          ),
        ),
      ],
    );
  }

  void _showChangePasswordDialog() {
    final oldPassCtrl = TextEditingController();
    final newPassCtrl = TextEditingController();
    final confirmPassCtrl = TextEditingController();

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text("Change Password"),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
             TextField(controller: oldPassCtrl, obscureText: true, decoration: const InputDecoration(labelText: "Old Password")),
             TextField(controller: newPassCtrl, obscureText: true, decoration: const InputDecoration(labelText: "New Password")),
             TextField(controller: confirmPassCtrl, obscureText: true, decoration: const InputDecoration(labelText: "Confirm New Password")),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text("CANCEL")),
          ElevatedButton(
            onPressed: () async {
              if (newPassCtrl.text != confirmPassCtrl.text) {
                ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text("Passwords do not match")));
                return;
              }
              if (newPassCtrl.text.length < 6) {
                ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text("Password too short")));
                return;
              }
              Navigator.pop(context); // Close dialog

              setState(() {
                _isSaving = true;
                _msg = ""; 
                _error = "";
              });

              final res = await ServiceLocator.profileService.changePassword(oldPassCtrl.text, newPassCtrl.text);
              
              setState(() {
                 _isSaving = false;
                 if (res['success'] == true) {
                    _msg = "Password changed successfully!";
                 } else {
                    _error = res['error'] ?? "Failed to change password";
                 }
              });
            },
            child: const Text("CHANGE"),
          )
        ],
      ),
    );
  }

  Widget _buildAvatarPresets() {
    return Wrap(
      spacing: 10,
      runSpacing: 10,
      children: _presetAvatars.map((path) {
        bool isSelected = _avatarController.text == path;
        return GestureDetector(
          onTap: () => setState(() => _avatarController.text = path),
          child: Container(
            width: 110,
            height: 110,
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: isSelected ? const Color(0xFF29B6F6) : const Color(0xFFDCE3F0),
                width: isSelected ? 4 : 2,
              ),
              color: Colors.white,
            ),
            padding: const EdgeInsets.all(4),
            child: ClipRRect(
              borderRadius: BorderRadius.circular(8),
              child: Image.asset(path, fit: BoxFit.cover,
                errorBuilder: (context, error, stackTrace) => const Icon(Icons.error),
              ),
            ),
          ),
        );
      }).toList(),
    );
  }

  Widget _buildPreview() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.center,
      children: [
        Container(
          width: 240,
          height: 240,
          decoration: BoxDecoration(
            shape: BoxShape.circle,
            border: Border.all(color: const Color(0xFFDCE3F0), width: 4),
            color: Colors.grey[200],
            boxShadow: [
              BoxShadow(
                color: Colors.black.withOpacity(0.1),
                blurRadius: 10,
                offset: const Offset(0, 5),
              )
            ]
          ),
          child: ClipOval(
            child: _avatarController.text.isNotEmpty
                ? Image.asset(_avatarController.text, fit: BoxFit.cover, 
                    errorBuilder: (c, e, s) => const Icon(Icons.person, size: 120))
                : const Center(child: Icon(Icons.person, size: 120, color: Colors.grey)),
          ),
        ),
        const SizedBox(height: 14),
        Text(
          _nameController.text.isEmpty ? "(No name yet)" : _nameController.text,
          style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 28, color: LobbyTheme.yellowGame),
        ),
        const SizedBox(height: 6),
        Text(
          _bioController.text.isEmpty ? "(No bio yet)" : _bioController.text,
          textAlign: TextAlign.center,
          style: const TextStyle(fontSize: 20, color: Colors.white70),
        ),
      ],
    );
  }

  // --- Helpers ---

  Widget _buildLabel(String text) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Text(text, style: const TextStyle(fontWeight: FontWeight.w600, color: Colors.white, fontSize: 24)),
    );
  }

  InputDecoration _inputDecoration(String hint) {
    return InputDecoration(
      contentPadding: const EdgeInsets.symmetric(vertical: 24, horizontal: 20),
      hintText: hint,
      filled: true,
      fillColor: Colors.white,
      border: OutlineInputBorder(borderRadius: BorderRadius.circular(12), borderSide: const BorderSide(color: Color(0xFFDCE3F0), width: 2)),
      enabledBorder: OutlineInputBorder(borderRadius: BorderRadius.circular(12), borderSide: const BorderSide(color: Color(0xFFDCE3F0), width: 2)),
      focusedBorder: OutlineInputBorder(borderRadius: BorderRadius.circular(12), borderSide: const BorderSide(color: Color(0xFF29B6F6), width: 2)),
    );
  }

  List<Shadow> _textOutlineShadows() {
    return const [
      Shadow(offset: Offset(-2, -2), color: Color(0xFF1F2A44)),
      Shadow(offset: Offset(2, -2), color: Color(0xFF1F2A44)),
      Shadow(offset: Offset(-2, 2), color: Color(0xFF1F2A44)),
      Shadow(offset: Offset(2, 2), color: Color(0xFF1F2A44)),
    ];
  }

  Widget _buildHeader() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(40),
      decoration: const BoxDecoration(
        color: Colors.transparent,
        border: Border(bottom: BorderSide(color: Colors.white24, width: 2)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                "Player Settings",
                style: GoogleFonts.luckiestGuy(
                  fontSize: 48,
                  color: LobbyTheme.yellowGame,
                  shadows: _textOutlineShadows(),
                ),
              ),
              IconButton(
                onPressed: () => Navigator.pop(context),
                icon: const Icon(Icons.close, color: Colors.white, size: 32),
              ),
            ],
          ),
          const SizedBox(height: 8),
          const Text(
            "Customize your profile so everyone recognizes you",
            style: TextStyle(color: Colors.white, fontSize: 20),
          ),
        ],
      ),
    );
  }
}
