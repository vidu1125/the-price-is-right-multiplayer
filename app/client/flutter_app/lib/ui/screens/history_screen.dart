import 'package:flutter/material.dart';
import '../../services/service_locator.dart';
import '../../models/match_record.dart';
import '../theme/lobby_theme.dart';
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
      backgroundColor: LobbyTheme.backgroundNavy,
      appBar: AppBar(
        title: Text("MATCH HISTORY", style: LobbyTheme.gameFont(fontSize: 22)),
        backgroundColor: Colors.transparent,
        elevation: 0,
        centerTitle: true,
      ),
      extendBodyBehindAppBar: true,
      body: Stack(
        children: [
          // Background Image
          Positioned.fill(
            child: Image.asset("assets/images/lobby-bg.png", fit: BoxFit.cover),
          ),
          Positioned.fill(child: Container(color: Colors.black.withOpacity(0.6))),

          SafeArea(
            child: FutureBuilder<List<MatchRecord>>(
              future: _historyFuture,
              builder: (context, snapshot) {
                if (snapshot.connectionState == ConnectionState.waiting) {
                  return const Center(child: CircularProgressIndicator(color: LobbyTheme.yellowGame));
                }
                
                if (snapshot.hasError) {
                  return Center(
                    child: Text("Error: ${snapshot.error}", 
                      style: const TextStyle(color: Colors.white70))
                  );
                }

                final records = snapshot.data ?? [];
                if (records.isEmpty) {
                  return Center(
                    child: Text("No match history found.", 
                      style: LobbyTheme.gameFont(color: Colors.white54))
                  );
                }

                return ListView.separated(
                  padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 20),
                  itemCount: records.length,
                  separatorBuilder: (context, index) => const SizedBox(height: 12),
                  itemBuilder: (context, index) {
                    final record = records[index];
                    return _buildHistoryItem(record);
                  },
                );
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHistoryItem(MatchRecord record) {
    return GestureDetector(
      onTap: () async {
        // Show loading dialog
        showDialog(
          context: context,
          barrierDismissible: false,
          builder: (context) => const Center(child: CircularProgressIndicator(color: LobbyTheme.yellowGame)),
        );

        try {
          final details = await ServiceLocator.historyService.viewMatchDetails(record.matchId);
          if (mounted) {
            Navigator.pop(context); // Close loading
            Navigator.push(
              context,
              MaterialPageRoute(
                builder: (context) => MatchDetailScreen(data: details),
              ),
            );
          }
        } catch (e) {
          if (mounted) {
            Navigator.pop(context); // Close loading
            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(content: Text("Failed to load details: $e"), backgroundColor: Colors.red),
            );
          }
        }
      },
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: LobbyTheme.rowBackground.withOpacity(0.3),
          borderRadius: BorderRadius.circular(14),
          border: Border.all(color: LobbyTheme.primaryDark, width: 2),
        ),
        child: Row(
          children: [
            // Winner/Loser Icon
            CircleAvatar(
              backgroundColor: record.isWinner ? Colors.green : Colors.redAccent,
              child: Icon(
                record.isWinner ? Icons.emoji_events : Icons.close,
                color: Colors.white,
              ),
            ),
            const SizedBox(width: 16),
            
            // Info
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text("Match #${record.matchId}", 
                    style: LobbyTheme.gameFont(fontSize: 16, color: LobbyTheme.yellowGame)),
                  const SizedBox(height: 4),
                  Text("${record.mode} • Score: ${record.score}", 
                    style: LobbyTheme.tableContentFont),
                ],
              ),
            ),

            // Date
            Column(
              crossAxisAlignment: CrossAxisAlignment.end,
              children: [
                Text(record.ranking, style: LobbyTheme.gameFont(fontSize: 14)),
                const SizedBox(height: 4),
                Text(
                  "${record.endedAt.day}/${record.endedAt.month} ${record.endedAt.hour}:${record.endedAt.minute.toString().padLeft(2, '0')}",
                  style: const TextStyle(fontSize: 10, color: Colors.white54),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
