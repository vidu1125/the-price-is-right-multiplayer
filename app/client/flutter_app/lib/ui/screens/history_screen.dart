import 'package:flutter/material.dart';
import '../../services/service_locator.dart';
import '../../models/match_record.dart';
import '../theme/history_theme.dart'; // Corrected import path
import 'match_detail_screen.dart';

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
          // Background giống bản web
          Positioned.fill(
            child: Image.asset("assets/images/lobby-bg.png", fit: BoxFit.cover),
          ),
          Positioned.fill(child: Container(color: Colors.black.withOpacity(0.4))), // Overlay for readability
          
          SafeArea(
            child: Center(
              child: SingleChildScrollView(
                padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 40),
                child: Column(
                  children: [
                    Text("THE PRICE IS RIGHT", style: HistoryTheme.titleStyle),
                    const SizedBox(height: 30),
                    
                    // Thẻ chứa bảng (Table Container) - Tương ứng .settings-card trong CSS
                    Container(
                      constraints: const BoxConstraints(maxWidth: 1000),
                      padding: const EdgeInsets.all(32),
                      decoration: HistoryTheme.glassCardDecoration,
                      child: Column(
                        children: [
                          _buildSummaryHeader(),
                          const SizedBox(height: 24),
                          const Divider(color: Colors.white24),
                          _buildTableHeaders(),
                          const SizedBox(height: 12),
                          
                          FutureBuilder<List<MatchRecord>>(
                            future: _historyFuture,
                            builder: (context, snapshot) {
                              if (snapshot.connectionState == ConnectionState.waiting) {
                                return const Padding(
                                  padding: EdgeInsets.all(40),
                                  child: CircularProgressIndicator(color: HistoryTheme.primaryBlue),
                                );
                              }
                              
                              if (snapshot.hasError) {
                                return Padding(
                                  padding: const EdgeInsets.all(16),
                                  child: Text("Error: ${snapshot.error}", style: const TextStyle(color: Colors.white70)),
                                );
                              }

                              final records = snapshot.data ?? [];
                              if (records.isEmpty) {
                                return const Padding(
                                  padding: EdgeInsets.all(16),
                                  child: Text("No records found.", style: TextStyle(color: Colors.white54)),
                                );
                              }

                              return Column(
                                children: records.map((r) => _buildTableRow(r)).toList(),
                              );
                            },
                          ),
                          
                          const SizedBox(height: 30),
                          _buildBackButton(),
                        ],
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

  // Header tóm tắt thông số (Matches, Top 1, Total Score)
  Widget _buildSummaryHeader() {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceAround,
      children: [
        _buildSummaryItem("MATCHES", "2"),
        _buildSummaryItem("TOP 1", "0"),
        _buildSummaryItem("TOTAL SCORE", "+21", color: Colors.greenAccent),
      ],
    );
  }

  Widget _buildSummaryItem(String label, String value, {Color color = Colors.white}) {
    return Column(
      children: [
        Text(label, style: HistoryTheme.columnHeaderStyle),
        const SizedBox(height: 8),
        Text(value, style: HistoryTheme.titleStyle.copyWith(fontSize: 24, color: color)),
      ],
    );
  }

  // Tiêu đề các cột trong bảng
  Widget _buildTableHeaders() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 16),
      child: Row(
        children: [
          Expanded(flex: 2, child: Center(child: Text("ID", style: HistoryTheme.columnHeaderStyle))),
          Expanded(flex: 3, child: Center(child: Text("MODE", style: HistoryTheme.columnHeaderStyle))),
          Expanded(flex: 3, child: Center(child: Text("FINAL SCORE", style: HistoryTheme.columnHeaderStyle))),
          Expanded(flex: 2, child: Center(child: Text("RANKING", style: HistoryTheme.columnHeaderStyle))),
          Expanded(flex: 2, child: Center(child: Text("ACTION", style: HistoryTheme.columnHeaderStyle))),
        ],
      ),
    );
  }

  // Một dòng dữ liệu trong bảng
  Widget _buildTableRow(MatchRecord record) {
    return Container(
      margin: const EdgeInsets.only(bottom: 8),
      padding: const EdgeInsets.symmetric(vertical: 12),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.05),
        borderRadius: BorderRadius.circular(10),
      ),
      child: Row(
        children: [
          Expanded(flex: 2, child: Center(child: Text("#${record.matchId}", style: const TextStyle(color: Colors.white70, fontWeight: FontWeight.bold)))),
          Expanded(flex: 3, child: Center(child: _buildModePill(record.mode))),
          Expanded(flex: 3, child: Center(child: Text(record.score.toString(), style: const TextStyle(color: Colors.redAccent, fontWeight: FontWeight.bold)))),
          
          // CỘT RANKING / STATUS ĐÃ CẬP NHẬT
          Expanded(
            flex: 2, 
            child: Center(child: _buildRankOrStatus(record))
          ),
          
          Expanded(flex: 2, child: Center(child: _buildViewButton(record))),
        ],
      ),
    );
  }

  // Widget xử lý hiển thị Thứ hạng hoặc Trạng thái đặc biệt
  Widget _buildRankOrStatus(MatchRecord record) {
    if (record.isForfeit == true) {
      return _buildStatusPill("FORFEIT", HistoryTheme.forfeitRed);
    } 
    
    if (record.isEliminated == true) {
      return _buildStatusPill("ELIMINATED", HistoryTheme.eliminatedOrange);
    }

    // Nếu không bị loại/bỏ cuộc thì hiện Rank Circle như cũ
    return _buildRankCircle(record.ranking);
  }

  // Widget bổ trợ cho nhãn trạng thái
  Widget _buildStatusPill(String text, Color color) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: color.withOpacity(0.2),
        borderRadius: BorderRadius.circular(6),
        border: Border.all(color: color, width: 1),
      ),
      child: Text(
        text, 
        style: HistoryTheme.statusTextStyle(color),
      ),
    );
  }

  Widget _buildModePill(String mode) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
      decoration: BoxDecoration(
        color: HistoryTheme.primaryBlue,
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: HistoryTheme.darkBlue),
      ),
      child: Text(mode.toUpperCase(), style: const TextStyle(color: Colors.white, fontSize: 10, fontWeight: FontWeight.bold)),
    );
  }

  Widget _buildRankCircle(String rank) {
    return Container(
      padding: const EdgeInsets.all(8),
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        border: Border.all(color: Colors.yellowAccent, width: 1),
      ),
      child: Text(rank, style: const TextStyle(color: Colors.yellowAccent, fontSize: 12, fontWeight: FontWeight.bold)),
    );
  }

  Widget _buildViewButton(MatchRecord record) {
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        backgroundColor: Colors.greenAccent.shade700,
        padding: const EdgeInsets.symmetric(horizontal: 16),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
      ),
      onPressed: () => _handleViewDetails(record),
      child: const Text("VIEW", style: TextStyle(color: Colors.white, fontSize: 10, fontWeight: FontWeight.bold)),
    );
  }

  Widget _buildBackButton() {
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        backgroundColor: Colors.orangeAccent,
        padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(12),
          side: const BorderSide(color: HistoryTheme.darkBlue, width: 2),
        ),
      ),
      onPressed: () => Navigator.pop(context),
      child: Text("BACK TO LOBBY", style: HistoryTheme.titleStyle.copyWith(fontSize: 16, color: Colors.white)),
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
