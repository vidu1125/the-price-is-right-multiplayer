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
      backgroundColor: const Color(0xFFE3F2FD), // Light blue background
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [Color(0xFFE3F2FD), Color(0xFFBBDEFB)],
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
          ),
        ),
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
    );
  }

  Widget _buildHeroSection() {
    return Container(
      padding: const EdgeInsets.all(24),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.12),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: Colors.white.withOpacity(0.2), width: 2),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text("LEARN THE RULES", 
            style: TextStyle(letterSpacing: 1.5, fontSize: 12, fontWeight: FontWeight.bold, color: Color(0xFF1F2A44))),
          const SizedBox(height: 8),
          Text(
            "The Price Is Right Tutorial",
            style: TutorialTheme.headerStyle(fontSize: 32),
          ),
          const SizedBox(height: 12),
          Text(
            "Quick recap of modes, mechanics, and scoring so you can jump into the lobby with confidence.",
            style: TutorialTheme.bodyStyle(),
          ),
          const SizedBox(height: 20),
          Row(
            children: [
              _buildHeroButton("Back to Lobby", isPrimary: true, onPressed: () => Navigator.pop(context)),
              const SizedBox(width: 12),
              _buildHeroButton("Go Back", isPrimary: false, onPressed: () => Navigator.pop(context)),
            ],
          ),
          const SizedBox(height: 30),
          Row(
            children: [
              _buildModePill("Scoring Mode", "4 - 6 players", "Highest total points wins"),
              const SizedBox(width: 10),
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
        style: TutorialTheme.headerStyle(fontSize: 14).copyWith(color: Colors.white),
      ),
    );
  }

  Widget _buildModePill(String title, String players, String goal) {
    return Expanded(
      child: Container(
        padding: const EdgeInsets.all(12),
        decoration: BoxDecoration(
          color: const Color(0xFF29B6F6).withOpacity(0.15),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: Colors.white.withOpacity(0.2)),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 14)),
            Text(players, style: const TextStyle(fontSize: 12, color: Colors.black54)),
            const SizedBox(height: 4),
            Text(goal, style: const TextStyle(fontSize: 12)),
          ],
        ),
      ),
    );
  }

  Widget _buildCarousel() {
    return Container(
      constraints: const BoxConstraints(minHeight: 400),
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.12),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: Colors.white.withOpacity(0.2), width: 2),
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
                  style: GoogleFonts.luckiestGuy(color: const Color(0xFF29B6F6), fontSize: 18),
                ),
              ),
              _buildNavBtn("NEXT →", _currentSection < sections.length - 1 ? () {
                _pageController.nextPage(duration: const Duration(milliseconds: 300), curve: Curves.easeInOut);
              } : null),
            ],
          ),
          const SizedBox(height: 10),
          Text("${_currentSection + 1} / ${sections.length}", style: GoogleFonts.luckiestGuy(color: const Color(0xFF29B6F6))),
          const SizedBox(height: 20),
          SizedBox(
            height: 300, // Fixed height for content area
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
        child: Text(text, style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 12)),
      ),
    );
  }

  Widget _renderSectionContent(Map<String, dynamic> section) {
    String type = section['type'];

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.25),
        borderRadius: BorderRadius.circular(12),
      ),
      child: _getContentWidget(type, section),
    );
  }

  Widget _getContentWidget(String type, Map<String, dynamic> section) {
    switch (type) {
      case "mechanics":
        return Column(
          children: mechanics.map((m) => ListTile(
            leading: const CircleAvatar(radius: 6, backgroundColor: Color(0xFF29B6F6)),
            title: Text(m['title']!, style: GoogleFonts.luckiestGuy(fontSize: 16)),
            subtitle: Text(m['detail']!),
          )).toList(),
        );
      case "round":
        var r = rounds[section['index']];
        return Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
              decoration: BoxDecoration(color: const Color(0xFF29B6F6), borderRadius: BorderRadius.circular(20)),
              child: Text("ROUND ${section['index'] + 1}", style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 12)),
            ),
            const SizedBox(height: 10),
            ...r['bullets'].map<Widget>((b) => Padding(
              padding: const EdgeInsets.only(bottom: 8),
              child: Row(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text("• ", style: TextStyle(fontWeight: FontWeight.bold)),
                  Expanded(child: Text(b)),
                ],
              ),
            )).toList(),
          ],
        );
      case "elimination":
        return ListView(children: elimination.map((e) => Text("• $e\n")).toList());
      case "bonusduel":
        return ListView(children: bonusDuel.map((e) => Text("• $e\n")).toList());
      case "badges":
        return ListView(children: badges.map((e) => Text("▪ $e\n")).toList());
      default:
        return const SizedBox();
    }
  }
}
