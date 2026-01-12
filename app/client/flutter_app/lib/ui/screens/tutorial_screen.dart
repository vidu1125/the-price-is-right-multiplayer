import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import '../theme/tutorial_theme.dart';

class TutorialPage extends StatefulWidget {
  const TutorialPage({super.key});

  @override
  State<TutorialPage> createState() => _TutorialPageState();
}

class _TutorialPageState extends State<TutorialPage> {
  final PageController _pageController = PageController();
  int _currentSection = 0;

  // Dữ liệu từ file JS của bạn
  final List<Map<String, dynamic>> sections = [
    {"id": "mechanics", "title": "Core Mechanics", "type": "mechanics"},
    {"id": "round1", "title": "Round 1 — Speed Challenge", "type": "round", "index": 0},
    {"id": "round2", "title": "Round 2 — The Bid", "type": "round", "index": 1},
    {"id": "round3", "title": "Round 3 — Bonus Wheel", "type": "round", "index": 2},
    {"id": "elimination", "title": "Elimination Flow", "type": "elimination"},
    {"id": "bonusduel", "title": "Bonus Duel", "type": "bonusduel"},
    {"id": "badges", "title": "Badges", "type": "badges"},
  ];

  final List<Map<String, String>> mechanics = [
    {"title": "Wager x2", "detail": "Activate before answering. Correct → +2x points. Wrong → -2x points (never below 0)."},
    {"title": "Perfect Guess", "detail": "Round 2 only. Exact product price → +2x that question's points."},
  ];

  final List<Map<String, dynamic>> rounds = [
    {
      "title": "Round 1 — Speed Challenge",
      "bullets": [
        "10 multiple-choice questions about prices or combos.",
        "15 seconds each question.",
        "Score = 200 - (answer_time / 15) * 100 (floored to nearest ten).",
      ],
    },
    {
      "title": "Round 2 — The Bid",
      "bullets": [
        "5 products, enter your numeric price guess.",
        "15 seconds each product.",
        "100 points to the closest guess without going over.",
        "If everyone is over, 50 points to the smallest overbid.",
      ],
    },
    {
      "title": "Round 3 — Bonus Wheel",
      "bullets": [
        "Spin values from 5 to 100.",
        "Up to 2 spins: after the first spin, choose to stop or spin again.",
        "If total ≤ 100: you score the total. If total > 100: score total - 100.",
        "Total exactly 100 or 200: you earn 100 points.",
      ],
    },
  ];

  final List<String> elimination = [
    "Applies only in Elimination Mode.",
    "After each main round the lowest total is eliminated.",
    "Round order: after R1 → 3 players, after R2 → 2 players, after R3 → 1 winner.",
  ];

  final List<String> bonusDuel = [
    "Used to break ties for elimination or for the final winner.",
    "Game shows a product and a reference price.",
    "Choose HIGHER or LOWER than the real price.",
    "Fastest correct wins; any wrong answer is eliminated.",
    "Ends when only one player remains.",
  ];

  final List<String> badges = [
    "Spin Master — reach a total spin of 100.",
    "Perfect Bidder — exact product price in Round 2.",
    "Sudden Death Winner — win a Bonus Duel.",
  ];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFE3F2FD),
      body: Stack(
        children: [
          // Background Image
          Positioned.fill(
            child: Image.asset("assets/images/lobby-bg.png", fit: BoxFit.cover),
          ),
          Positioned.fill(
            child: Container(
              decoration: BoxDecoration(
                gradient: LinearGradient(
                  begin: Alignment.topCenter,
                  end: Alignment.bottomCenter,
                  colors: [
                    const Color(0xFF0D47A1).withOpacity(0.7), 
                    const Color(0xFF29B6F6).withOpacity(0.5),
                  ],
                ),
              ),
            ),
          ),
          
          Container(
            child: SingleChildScrollView(
              padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 40),
              child: Column(
                children: [
                  _buildHeroSection(),
                  const SizedBox(height: 40),
                  _buildCarousel(),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHeroSection() {
    return Container(
      padding: const EdgeInsets.all(30),
      // Use glassDecoration from theme
      decoration: TutorialTheme.glassDecoration(opacity: 0.2), 
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text("LEARN THE RULES", style: TutorialTheme.gameFont(size: 18, color: TutorialTheme.brightYellow)),
          const SizedBox(height: 10),
          Text(
            "The Price Is Right",
            style: GoogleFonts.luckiestGuy(fontSize: 52, color: Colors.white, shadows: TutorialTheme.gameShowShadow(offset: 3)),
          ),
          const SizedBox(height: 12),
          Text(
            "Quick recap of modes, mechanics, and scoring.",
            style: TextStyle(fontFamily: 'Parkinsans', fontSize: 18, color: Colors.white.withOpacity(0.9)),
          ),
          const SizedBox(height: 25),
          Row(
            children: [
              _buildHeroButton("Back to Lobby", isPrimary: true, onPressed: () => Navigator.pop(context)),
              const SizedBox(width: 15),
              _buildHeroButton("Go Back", isPrimary: false, onPressed: () => Navigator.pop(context)),
            ],
          ),
          const SizedBox(height: 35),
          Row(
            children: [
              _buildModePill("Scoring Mode", "4 - 6 players", "Highest total points wins"),
              const SizedBox(width: 15),
              _buildModePill("Elimination Mode", "Exactly 4 players", "Be the last player"),
            ],
          )
        ],
      ),
    );
  }

  Widget _buildHeroButton(String text, {required bool isPrimary, required VoidCallback onPressed}) {
    return ElevatedButton(
      onPressed: onPressed,
      style: ElevatedButton.styleFrom(
        backgroundColor: isPrimary ? TutorialTheme.primaryBlue : Colors.white.withOpacity(0.1),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(14),
          side: const BorderSide(color: TutorialTheme.darkBlue, width: 2),
        ),
        elevation: 6,
      ),
      child: Text(
        text.toUpperCase(),
        style: GoogleFonts.luckiestGuy(fontSize: 18, color: Colors.white),
      ),
    );
  }

  Widget _buildModePill(String title, String players, String goal) {
    return Expanded(
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: const Color(0xFF29B6F6).withOpacity(0.2),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: const Color(0xFF29B6F6), width: 2),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: GoogleFonts.luckiestGuy(fontSize: 20, color: const Color(0xFF29B6F6))),
            const SizedBox(height: 5),
            Text(players, style: const TextStyle(fontFamily: 'Parkinsans', fontSize: 16, color: Colors.white70)),
            const SizedBox(height: 4),
            Text(goal, style: const TextStyle(fontFamily: 'Parkinsans', fontSize: 16, color: Colors.white)),
          ],
        ),
      ),
    );
  }

  Widget _buildCarousel() {
    return Container(
      constraints: const BoxConstraints(minHeight: 500),
      padding: const EdgeInsets.all(24),
      decoration: BoxDecoration(
        color: const Color(0xFF1E1E2F).withOpacity(0.95),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: const Color(0xFFFFDE59), width: 3),
         boxShadow: [
          BoxShadow(color: Colors.black.withOpacity(0.5), blurRadius: 15, offset: const Offset(0, 5))
        ]
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              _buildNavBtn("← PREV", _currentSection > 0 ? () {
                _pageController.previousPage(duration: const Duration(milliseconds: 300), curve: Curves.easeInOut);
              } : null),
              Expanded(
                child: Text(
                  sections[_currentSection]['title'].toUpperCase(),
                  textAlign: TextAlign.center,
                  style: GoogleFonts.luckiestGuy(color: const Color(0xFFFFDE59), fontSize: 28),
                ),
              ),
              _buildNavBtn("NEXT →", _currentSection < sections.length - 1 ? () {
                _pageController.nextPage(duration: const Duration(milliseconds: 300), curve: Curves.easeInOut);
              } : null),
            ],
          ),
          const SizedBox(height: 15),
          Text("${_currentSection + 1} / ${sections.length}", style: GoogleFonts.luckiestGuy(color: Colors.white54, fontSize: 20)),
          const SizedBox(height: 25),
          SizedBox(
            height: 400, // Increased height
            child: PageView.builder(
              controller: _pageController,
              onPageChanged: (index) => setState(() => _currentSection = index),
              itemCount: sections.length,
              itemBuilder: (context, index) => _renderSectionContent(sections[index]),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildNavBtn(String text, VoidCallback? onPressed) {
    return Opacity(
      opacity: onPressed == null ? 0.4 : 1.0,
      child: TextButton(
        onPressed: onPressed,
        style: TextButton.styleFrom(
          backgroundColor: const Color(0xFF29B6F6),
          padding: const EdgeInsets.symmetric(horizontal: 12),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(12),
            side: const BorderSide(color: Color(0xFF1F2A44), width: 2),
          ),
        ),
        child: Text(text, style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 16)),
      ),
    );
  }

  Widget _renderSectionContent(Map<String, dynamic> section) {
    return Container(
      padding: const EdgeInsets.all(30),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.95), // Nền trắng sữa sang trọng
        borderRadius: BorderRadius.circular(30),
        boxShadow: [
          BoxShadow(color: Colors.black26, blurRadius: 20, offset: const Offset(0, 10))
        ],
      ),
      child: Column(
        children: [
          // Thanh trang trí nhỏ phía trên
          Container(width: 50, height: 5, decoration: BoxDecoration(color: Colors.grey[300], borderRadius: BorderRadius.circular(10))),
          const SizedBox(height: 20),
          Expanded(child: _getContentWidget(section['type'], section)),
        ],
      ),
    );
  }

  Widget _getContentWidget(String type, Map<String, dynamic> section) {
    final Color titleColor = const Color(0xFF1A237E); // Xanh Navy đậm

    switch (type) {
      case "mechanics":
        return Column(
          children: mechanics.map((m) => Container(
            margin: const EdgeInsets.only(bottom: 20),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Icon(Icons.stars_rounded, color: TutorialTheme.primaryBlue, size: 36), // Increased from 28
                const SizedBox(width: 15),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(m['title']!, style: GoogleFonts.luckiestGuy(fontSize: 26, color: titleColor)), // Increased from 22
                      const SizedBox(height: 5),
                      Text(m['detail']!, style: TutorialTheme.detailTextStyle),
                    ],
                  ),
                ),
              ],
            ),
          )).toList(),
        );
      case "round":
        var r = rounds[section['index']];
        return Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            ...r['bullets'].map<Widget>((b) => Padding(
              padding: const EdgeInsets.only(bottom: 16),
              child: Row(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text("⚡", style: TextStyle(fontSize: 28)), // Increased from 20
                  const SizedBox(width: 12),
                  Expanded(child: Text(b, style: TutorialTheme.detailTextStyle)),
                ],
              ),
            )).toList(),
          ],
        );
      case "elimination":
        return Column(
          children: elimination.map((e) => Padding(
            padding: const EdgeInsets.only(bottom: 16),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text("💀", style: TextStyle(fontSize: 28)),
                const SizedBox(width: 12),
                Expanded(child: Text(e, style: TutorialTheme.detailTextStyle)),
              ],
            ),
          )).toList(),
        );
      case "bonusduel":
        return Column(
          children: bonusDuel.map((e) => Padding(
            padding: const EdgeInsets.only(bottom: 16),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text("⚔️", style: TextStyle(fontSize: 28)),
                const SizedBox(width: 12),
                Expanded(child: Text(e, style: TutorialTheme.detailTextStyle)),
              ],
            ),
          )).toList(),
        );
      case "badges":
        return Column(
          children: badges.map((e) => Padding(
            padding: const EdgeInsets.only(bottom: 16),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text("🏅", style: TextStyle(fontSize: 28)),
                const SizedBox(width: 12),
                Expanded(child: Text(e, style: TutorialTheme.detailTextStyle)),
              ],
            ),
          )).toList(),
        );
      default:
        return const SizedBox();
    }
  }
}
