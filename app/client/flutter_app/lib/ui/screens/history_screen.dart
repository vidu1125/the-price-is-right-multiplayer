import 'package:flutter/material.dart';
import '../../services/service_locator.dart';
import '../../models/match_record.dart';
import '../theme/history_theme.dart'; // Corrected import path
import 'match_detail_screen.dart';
import 'package:google_fonts/google_fonts.dart';

class HistoryScreen extends StatefulWidget {
  const HistoryScreen({super.key});

  @override
  State<HistoryScreen> createState() => _HistoryScreenState();
}

class _HistoryScreenState extends State<HistoryScreen> {
  late Future<List<MatchRecord>> _historyFuture;

  @override
  void initState() {
    super.initState();
    _historyFuture = ServiceLocator.historyService.viewHistory(forceUpdate: true);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          // Background
          Positioned.fill(child: Image.asset("assets/images/lobby-bg.png", fit: BoxFit.cover)),
          Positioned.fill(child: Container(color: Colors.black.withOpacity(0.5))),

          LayoutBuilder(
            builder: (context, constraints) {
              final double sw = constraints.maxWidth;
              final double cardWidth = sw > 1600 ? 1500 : sw * 0.95;

              return Center(
                child: Container(
                  width: cardWidth,
                  height: constraints.maxHeight * 0.85,
                  decoration: HistoryTheme.glassCardDecoration,
                  padding: const EdgeInsets.all(32),
                  child: Column(
                    children: [
                      Text("MATCH HISTORY", style: HistoryTheme.titleStyle.copyWith(fontSize: 60)),
                      const SizedBox(height: 32),
                      
                      _buildOverallStats(),
                      const SizedBox(height: 32),
                      
                      _buildTableHeader(),
                      const Divider(color: Colors.white24, thickness: 1),
                      
                      Expanded(
                        child: FutureBuilder<List<MatchRecord>>(
                          future: _historyFuture,
                          builder: (context, snapshot) {
                            if (snapshot.connectionState == ConnectionState.waiting) {
                              return const Center(child: CircularProgressIndicator(color: HistoryTheme.primaryGold));
                            }
                            final records = snapshot.data ?? [];
                            if (records.isEmpty) {
                               return Center(child: Text("No records found.", style: GoogleFonts.luckiestGuy(color: Colors.white54, fontSize: 24)));
                            }
                            return ListView.separated(
                              padding: const EdgeInsets.only(top: 8),
                              itemCount: records.length,
                              separatorBuilder: (_, __) => const SizedBox(height: 12),
                              itemBuilder: (context, index) => _buildDataRow(records[index]),
                            );
                          },
                        ),
                      ),
                      
                      const SizedBox(height: 24),
                      _buildBackButton(),
                    ],
                  ),
                ),
              );
            }
          ),
        ],
      ),
    );
  }

  Widget _buildOverallStats() {
    return Container(
      padding: const EdgeInsets.symmetric(vertical: 20, horizontal: 40),
      decoration: HistoryTheme.statsPanelDecoration,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: [
          _statItem("MATCHES", "2", HistoryTheme.primaryBlue),
          _statItem("TOP 1", "0", HistoryTheme.primaryGold),
          _statItem("TOTAL SCORE", "+21", HistoryTheme.successGreen),
        ],
      ),
    );
  }

  Widget _statItem(String label, String value, Color color) {
    return Column(
      children: [
        Text(label, style: GoogleFonts.luckiestGuy(color: Colors.white70, fontSize: 20)),
        const SizedBox(height: 8),
        Text(value, style: HistoryTheme.statValueStyle.copyWith(color: color, fontSize: 48)),
      ],
    );
  }

  Widget _buildTableHeader() {
    final textStyle = GoogleFonts.luckiestGuy(color: HistoryTheme.primaryGold, fontSize: 24);
    return Padding(
      padding: EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(
        children: [
          Expanded(flex: 2, child: Center(child: Text("ID", style: textStyle))),
          Expanded(flex: 3, child: Center(child: Text("MODE", style: textStyle))),
          Expanded(flex: 3, child: Center(child: Text("FINAL SCORE", style: textStyle))),
          Expanded(flex: 2, child: Center(child: Text("RANKING", style: textStyle))),
          Expanded(flex: 2, child: Center(child: Text("ACTION", style: textStyle))),
        ],
      ),
    );
  }

  Widget _buildDataRow(MatchRecord record) {
    return Container(
      padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 16),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.05),
        borderRadius: BorderRadius.circular(12),
      ),
      child: Row(
        children: [
          Expanded(flex: 2, child: Center(child: Text("#${record.matchId}", style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 24)))),
          Expanded(flex: 3, child: _modePill("SCORING")),
          // Mocking +21 score for now as requested in UI design
          Expanded(flex: 3, child: Center(child: Text("+${record.score}", style: GoogleFonts.luckiestGuy(color: HistoryTheme.successGreen, fontSize: 36, shadows: HistoryTheme.textOutlineShadow)))),
          Expanded(flex: 2, child: Center(child: _rankCircle("${record.matchId % 3 + 1}"))), // Mock ranking logic
          Expanded(flex: 2, child: _viewButton(record)),
        ],
      ),
    );
  }

  Widget _modePill(String text) {
    return Center(
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
        decoration: BoxDecoration(
          color: HistoryTheme.primaryBlue,
          borderRadius: BorderRadius.circular(8),
          border: Border.all(color: Colors.white, width: 1.5),
        ),
        child: Text(text, style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 20)),
      ),
    );
  }

  Widget _rankCircle(String rank) {
    return Container(
      width: 45,
      height: 45,
      alignment: Alignment.center,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        border: Border.all(color: HistoryTheme.primaryGold, width: 2),
        color: Colors.black26,
      ),
      child: Text(rank, style: GoogleFonts.luckiestGuy(color: HistoryTheme.primaryGold, fontSize: 24)),
    );
  }

  Widget _viewButton(MatchRecord record) {
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        backgroundColor: HistoryTheme.successGreen,
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 12),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
      ),
      onPressed: () => _handleViewDetails(record),
      child: Text("VIEW", style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 18)),
    );
  }

  Widget _buildBackButton() {
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        backgroundColor: Colors.orange,
        padding: const EdgeInsets.symmetric(horizontal: 32, vertical: 20),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      ),
      onPressed: () => Navigator.pop(context),
      child: Text("BACK TO LOBBY", style: GoogleFonts.luckiestGuy(color: Colors.white, fontSize: 24)),
    );
  }


  Future<void> _handleViewDetails(MatchRecord record) async {
    // Logic gọi service giữ nguyên không đổi
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => const Center(child: CircularProgressIndicator(color: HistoryTheme.primaryBlue)),
    );

    try {
      final details = await ServiceLocator.historyService.viewMatchDetails(record.matchId);
      // Inject data from record to details
      details['score'] = record.score;
      details['display_mode'] = record.mode;
      
      if (mounted) {
        Navigator.pop(context);
        Navigator.push(context, MaterialPageRoute(builder: (context) => MatchDetailScreen(data: details)));
      }
    } catch (e) {
      if (mounted) {
        Navigator.pop(context);
        ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("Error: $e")));
      }
    }
  }
}
