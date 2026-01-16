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
    {"id": "round1", "title": "Round 1 — Speed Challenge", "type": "round", "index": 0},
    {"id": "round2", "title": "Round 2 — The Bid", "type": "round", "index": 1},
    {"id": "round3", "title": "Round 3 — Bonus Wheel", "type": "round", "index": 2},
    {"id": "bonus", "title": "Round Bonus", "type": "round", "index": 3},
  ];


  final List<Map<String, dynamic>> rounds = [
    {
      "title": "Round 1 — Speed Challenge",
      "bullets": [
        "Answer fast. The quicker you response, the higher is your score",
        "15 seconds each question.",
        "Score = 200 - (answer_time / 15) * 100 (floored to nearest ten).",
      ],
    },
    {
      "title": "Round 2 — The Bid",
      "bullets": [
        "Make a bold estimate. Closest answer wins big points",
        "15 seconds each product.",
        "100 points to the closest guess without going over.",
        "If everyone is over, 50 points to the smallest overbid.",
      ],
    },
    {
      "title": "Round 3 — Bonus Wheel",
      "bullets": [
        "Spin your luck and watch the game turn in an instance",
        "Up to 2 spins: after the first spin, choose to stop or spin again.",
        "If total ≤ 100: you score the total. If total > 100: score total - 100.",
        "Total exactly 100 or 200: you earn 100 points.",
      ],
    },
    {
      "title": "Round bonus",
      "bullets": [
        "One draw decides who stays",
        "Trigger in elimination mode after a round if more than two players are tied at the lowest scores",
         "Trigger in scoring mode in the final round if more than two players are tied at the highest scores"
      ],
    },
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
              color: Colors.black.withOpacity(0.5), // Match Lobby overlay
            ),
          ),
          
          // Unified Full-Screen Panel
          Positioned.fill(
            child: LayoutBuilder(
              builder: (context, constraints) {
                return Center(
                  child: Container(
                    width: constraints.maxWidth * 0.85, 
                    height: constraints.maxHeight * 0.75, 
                    margin: const EdgeInsets.all(24),
                    padding: const EdgeInsets.all(40),
                    decoration: BoxDecoration(
                      color: const Color(0xB31F2A44),
                      borderRadius: BorderRadius.circular(24),
                      border: Border.all(color: const Color(0xFF1F2A44), width: 4),
                      boxShadow: [
                        BoxShadow(
                          color: Colors.black.withOpacity(0.5),
                          blurRadius: 30,
                          offset: const Offset(0, 10),
                        )
                      ],
                    ),
                    child: constraints.maxWidth > 1000 
                      ? Row(
                          crossAxisAlignment: CrossAxisAlignment.center,
                          children: [
                            Expanded(
                                flex: 5,
                                child: SingleChildScrollView(child: _buildHeroSection())
                              ),
                            Container(
                              width: 2, 
                              decoration: BoxDecoration(
                                gradient: LinearGradient(
                                  begin: Alignment.topCenter,
                                  end: Alignment.bottomCenter,
                                  colors: [Colors.transparent, Colors.white.withOpacity(0.2), Colors.transparent],
                                )
                              ),
                              margin: const EdgeInsets.symmetric(horizontal: 32)
                            ),
                            Expanded(
                              flex: 5,
                              child: _buildCarousel()
                            ),
                          ],
                        )
                      : SingleChildScrollView(
                          child: Column(
                            children: [
                              _buildHeroSection(),
                              const SizedBox(height: 32),
                              const Divider(color: Colors.white12, thickness: 2),
                              const SizedBox(height: 32),
                              _buildCarousel(),
                            ],
                          ),
                        ),
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHeroSection() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        Text("LEARN THE RULES", style: TutorialTheme.gameFont(size: 26, color: TutorialTheme.brightYellow)),
        const SizedBox(height: 10),
        Text(
          "The Price Is Right",
          style: GoogleFonts.luckiestGuy(fontSize: 68, color: Colors.white, shadows: TutorialTheme.gameShowShadow(offset: 4)),
        ),
        const SizedBox(height: 12),
        Text(
          "Quick recap of modes, mechanics, and scoring.",
          style: TextStyle(fontFamily: 'Parkinsans', fontSize: 24, color: Colors.white.withOpacity(0.9)),
        ),
        const SizedBox(height: 25),
        // Single Back Button
        Container(
          width: double.infinity,
          constraints: const BoxConstraints(maxWidth: 250),
          child: _buildHeroButton("Back to Lobby", isPrimary: true, onPressed: () => Navigator.pop(context)),
        ),
        const SizedBox(height: 35),
        Column(
          children: [
            _buildModePill("Scoring Mode", "4 - 6 players", "Highest total points wins"),
            const SizedBox(height: 15),
            _buildModePill("Elimination Mode", "Exactly 4 players", "Be the last player"),
          ],
        )
      ],
    );
  }

  Widget _buildHeroButton(String text, {required bool isPrimary, required VoidCallback onPressed}) {
    return ElevatedButton(
      onPressed: onPressed,
      style: ElevatedButton.styleFrom(
        padding: const EdgeInsets.symmetric(vertical: 20, horizontal: 24),
        backgroundColor: isPrimary ? TutorialTheme.primaryBlue : Colors.white.withOpacity(0.1),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(14),
          side: const BorderSide(color: TutorialTheme.darkBlue, width: 2),
        ),
        elevation: 6,
      ),
      child: Text(
        text.toUpperCase(),
        style: GoogleFonts.luckiestGuy(fontSize: 26, color: Colors.white),
      ),
    );
  }

  Widget _buildModePill(String title, String players, String goal) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: const Color(0xFF29B6F6).withOpacity(0.15),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: const Color(0xFF29B6F6).withOpacity(0.5), width: 1.5),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              const Icon(Icons.info_outline, color: Color(0xFF29B6F6), size: 30),
              const SizedBox(width: 10),
              Text(title, style: GoogleFonts.luckiestGuy(fontSize: 30, color: const Color(0xFF29B6F6))),
            ],
          ),
          const SizedBox(height: 10),
          Text(players, style: const TextStyle(fontFamily: 'Parkinsans', fontSize: 22, color: Colors.white70)),
          const SizedBox(height: 6),
          Text(goal, style: const TextStyle(fontFamily: 'Parkinsans', fontSize: 22, color: Colors.white)),
        ],
      ),
    );
  }

  Widget _buildCarousel() {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min, // Wrap content height
        mainAxisAlignment: MainAxisAlignment.center,
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
                style: GoogleFonts.luckiestGuy(color: const Color(0xFFFFDE59), fontSize: 42),
              ),
            ),
            _buildNavBtn("NEXT →", _currentSection < sections.length - 1 ? () {
              _pageController.nextPage(duration: const Duration(milliseconds: 300), curve: Curves.easeInOut);
            } : null),
          ],
        ),
        const SizedBox(height: 15),
        Text("${_currentSection + 1} / ${sections.length}", style: GoogleFonts.luckiestGuy(color: Colors.white54, fontSize: 28)),
        const SizedBox(height: 25),
        SizedBox(
          height: 450, // Keep fixed height for consistency
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
        child: Text(text, style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 24)),
      ),
    );
  }

  Widget _renderSectionContent(Map<String, dynamic> section) {
    return Container(
      padding: const EdgeInsets.all(30),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.96), // Match Settings
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
          // Thanh trang trí nhỏ phía trên
          Container(width: 50, height: 5, decoration: BoxDecoration(color: Colors.grey[300], borderRadius: BorderRadius.circular(10))),
          const SizedBox(height: 20),
          Expanded(child: _getContentWidget(section['type'], section)),
        ],
      ),
    );
  }

  Widget _getContentWidget(String type, Map<String, dynamic> section) {
    switch (type) {
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
                  const Text("⚡", style: TextStyle(fontSize: 42)), // Increased
                  const SizedBox(width: 12),
                  Expanded(child: Text(b, style: TutorialTheme.detailTextStyle)),
                ],
              ),
            )).toList(),
          ],
        );
      default:
        return const SizedBox();
    }
  }
}
