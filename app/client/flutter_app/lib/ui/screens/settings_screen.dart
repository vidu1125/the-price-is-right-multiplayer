import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import '../theme/tutorial_theme.dart';
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
  final int _maxName = 50;

  final List<String> _presetAvatars = [
    "https://via.placeholder.com/150/0000FF/FFFFFF?text=Mushroom", 
    "https://via.placeholder.com/150/FFD700/000000?text=Crown",
  ];

  @override
  void initState() {
    super.initState();
    // Reset message khi người dùng thay đổi input
    _nameController.addListener(() => setState(() => _msg = ""));
    _avatarController.addListener(() => setState(() => _msg = ""));
    _bioController.addListener(() => setState(() => _msg = ""));
  }

  Future<void> _onSubmit() async {
    setState(() {
      _isSaving = true;
      _msg = "";
      _error = "";
    });

    final name = _nameController.text.trim();
    final avatar = _avatarController.text.trim();

    // Validation logic từ file JS
    if (name.isEmpty) {
      setState(() {
        _error = "Display name cannot be empty";
        _isSaving = false;
      });
      return;
    }

    if (avatar.isNotEmpty && !Uri.parse(avatar).isAbsolute) {
      setState(() {
        _error = "Avatar must be a valid URL";
        _isSaving = false;
      });
      return;
    }

    try {
      final res = await ServiceLocator.profileService.updateProfile(
        UserProfile(name: name, avatar: avatar, bio: _bioController.text)
      );
      if (res['success'] == true) {
        setState(() => _msg = "Profile updated successfully!");
      } else {
        setState(() => _msg = res['error'] ?? "An error occurred");
      }
    } catch (err) {
      setState(() => _msg = "Could not update profile");
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
              padding: const EdgeInsets.all(24),
              child: Container(
                constraints: const BoxConstraints(maxWidth: 980),
                decoration: BoxDecoration(
                  color: Colors.white.withOpacity(0.96),
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(color: const Color(0xFF1F2A44), width: 2),
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

  Widget _buildHeader() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(28),
      decoration: const BoxDecoration(
        gradient: LinearGradient(colors: [Color(0xFF29B6F6), Color(0xFF00ACC1)]),
        border: Border(bottom: BorderSide(color: Color(0xFF1F2A44), width: 2)),
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
                  fontSize: 36,
                  color: Colors.white,
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
            style: TextStyle(color: Colors.white, fontSize: 16),
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
        _buildLabel("Avatar (URL)"),
        TextField(
          controller: _avatarController,
          onChanged: (val) => setState(() {}), // Update preview
          decoration: _inputDecoration("https://..."),
        ),
        const SizedBox(height: 10),
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
          child: ElevatedButton(
            onPressed: _isSaving ? null : _onSubmit,
            style: ElevatedButton.styleFrom(
              backgroundColor: const Color(0xFF29B6F6),
              padding: const EdgeInsets.symmetric(vertical: 20),
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(14),
                side: const BorderSide(color: Color(0xFF1F2A44), width: 2),
              ),
              elevation: 6,
            ),
            child: Text(
              _isSaving ? "SAVING..." : "SAVE CHANGES",
              style: GoogleFonts.luckiestGuy(
                fontSize: 22,
                color: Colors.white,
                shadows: _textOutlineShadows(),
              ),
            ),
          ),
        ),
      ],
    );
  }

  Widget _buildAvatarPresets() {
    return Wrap(
      spacing: 10,
      children: _presetAvatars.map((url) {
        bool isSelected = _avatarController.text == url;
        return GestureDetector(
          onTap: () => setState(() => _avatarController.text = url),
          child: Container(
            width: 64,
            height: 64,
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: isSelected ? const Color(0xFF29B6F6) : const Color(0xFFDCE3F0),
                width: isSelected ? 3 : 2,
              ),
              image: DecorationImage(image: NetworkImage(url), fit: BoxFit.cover),
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
          width: 160,
          height: 160,
          decoration: BoxDecoration(
            shape: BoxShape.circle,
            border: Border.all(color: const Color(0xFFDCE3F0), width: 2),
            color: Colors.grey[200],
          ),
          child: ClipOval(
            child: _avatarController.text.isNotEmpty
                ? Image.network(_avatarController.text, fit: BoxFit.cover, 
                    errorBuilder: (c, e, s) => const Icon(Icons.person, size: 80))
                : const Center(child: Icon(Icons.person, size: 80, color: Colors.grey)),
          ),
        ),
        const SizedBox(height: 14),
        Text(
          _nameController.text.isEmpty ? "(No name yet)" : _nameController.text,
          style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 18, color: Color(0xFF1F2A44)),
        ),
        const SizedBox(height: 6),
        Text(
          _bioController.text.isEmpty ? "(No bio yet)" : _bioController.text,
          textAlign: TextAlign.center,
          style: const TextStyle(fontSize: 14, color: Color(0xFF65748B)),
        ),
      ],
    );
  }

  // --- Helpers ---

  Widget _buildLabel(String text) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Text(text, style: const TextStyle(fontWeight: FontWeight.w600, color: Color(0xFF1F2A44))),
    );
  }

  InputDecoration _inputDecoration(String hint) {
    return InputDecoration(
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
}
