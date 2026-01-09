import 'package:flutter/material.dart';
import 'core/config.dart';
import 'services/service_locator.dart';
import 'ui/screens/login_screen.dart';
import 'ui/screens/lobby_screen.dart';
import 'ui/screens/history_screen.dart';
import 'ui/screens/register_screen.dart';

final GlobalKey<NavigatorState> navigatorKey = GlobalKey<NavigatorState>();

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  
  // 1. Initialize all services
  ServiceLocator.init();
  
  // 2. Setup global auth redirect
  ServiceLocator.tcpClient.onUnauthenticated = () async {
    await ServiceLocator.authService.clearAuth();
    navigatorKey.currentState?.pushNamedAndRemoveUntil('/login', (route) => false);
  };

  // 3. Connect and Bootstrap
  try {
    await ServiceLocator.tcpClient.connect(AppConfig.serverHost, AppConfig.serverPort);
    
    final authState = await ServiceLocator.authService.getAuthState();
    final sid = authState["sessionId"];
    if (sid != null) {
      print("🔄 Found session, attempting bootstrap reconnect...");
      await ServiceLocator.authService.reconnect(sid);
    }
  } catch (e) {
    print("❌ Startup failed: $e");
  }

  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'The Price Is Right',
      debugShowCheckedModeBanner: false,
      navigatorKey: navigatorKey,
      home: const LoginScreen(),
      routes: {
        '/login': (context) => const LoginScreen(),
        '/register': (context) => const RegisterScreen(),
        '/lobby': (context) => const LobbyScreen(),
        '/history': (context) => const HistoryScreen(),
        '/tutorial': (context) => Scaffold(appBar: AppBar(title: const Text("Tutorial")), body: const Center(child: Text("Tutorial Screen"))),
      },
    );
  }
}
