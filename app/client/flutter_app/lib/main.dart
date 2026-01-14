import 'package:flutter/material.dart';
import 'models/room.dart';
import 'core/config.dart';
import 'services/service_locator.dart';
import 'ui/screens/login_screen.dart';
import 'ui/screens/lobby_screen.dart';
import 'ui/screens/history_screen.dart';
import 'ui/screens/register_screen.dart';
import 'ui/screens/tutorial_screen.dart';
import 'ui/screens/settings_screen.dart';
import 'ui/screens/room_screen.dart';
import 'ui/screens/game_container_screen.dart';

final GlobalKey<NavigatorState> navigatorKey = GlobalKey<NavigatorState>();

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  
  // 1. Initialize all services
  ServiceLocator.init();
  
  // 2. Setup global auth redirect
  ServiceLocator.tcpClient.onUnauthenticated = () async {
    await ServiceLocator.authService.clearAuth();
    ServiceLocator.tcpClient.disconnect();
    navigatorKey.currentState?.pushNamedAndRemoveUntil('/login', (route) => false);
  };

  // 3. Connect and Bootstrap
  String initialRoute = '/login';
  try {
    await ServiceLocator.tcpClient.connect(AppConfig.serverHost, AppConfig.serverPort);
    
    final authState = await ServiceLocator.authService.getAuthState();
    final sid = authState["sessionId"];
    if (sid != null) {
      print("🔄 Found session, attempting bootstrap reconnect...");
      final res = await ServiceLocator.authService.reconnect(sid);
      if (res["success"] == true) {
         initialRoute = '/lobby';
         print("✅ Bootstrap reconnect successful. Navigating to Lobby.");
      } else {
         print("⚠️ Bootstrap reconnect failed: ${res['error']}");
         // Auto clear is handled in reconnect() if it fails with invalid session, 
         // but if it failed due to connection error, maybe we stay on login.
      }
    }
  } catch (e) {
    print("❌ Startup failed: $e");
  }

  runApp(MyApp(initialRoute: initialRoute));
}

class MyApp extends StatelessWidget {
  final String initialRoute;
  const MyApp({super.key, required this.initialRoute});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'The Price Is Right',
      debugShowCheckedModeBanner: false,
      navigatorKey: navigatorKey,
      initialRoute: initialRoute,
      builder: (context, child) {
        precacheImage(const AssetImage("assets/images/lobby-bg.png"), context);
        precacheImage(const AssetImage("assets/images/waitingroom.png"), context);
        precacheImage(const AssetImage("assets/images/default-mushroom.jpg"), context);
        return child!;
      },
      routes: {
        '/login': (context) => const LoginScreen(),
        '/register': (context) => const RegisterScreen(),
        '/lobby': (context) => const LobbyScreen(),
        '/history': (context) => const HistoryScreen(),
        '/tutorial': (context) => const TutorialPage(),
        '/settings': (context) => const PlayerSettingsPage(),
        '/game': (context) => const GameContainerScreen(),
        '/room': (context) {
           final args = ModalRoute.of(context)!.settings.arguments as Map<String, dynamic>;
           return RoomScreen(
             room: args['room'] as dynamic, // Expecting Room object
             initialIsHost: args['initialIsHost'] as bool? ?? false
           );
        },
      },
    );
  }
}
