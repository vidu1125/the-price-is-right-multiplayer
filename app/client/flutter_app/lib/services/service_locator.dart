import '../network/tcp_client.dart';
import '../network/dispatcher.dart';
import 'auth_service.dart';
import 'history_service.dart';
import 'profile_service.dart';
import 'room_service.dart';
import 'game_service.dart';
import 'game_state_service.dart';
import 'friend_service.dart';
import 'bonus_service.dart';

/// Centralized access to all singletons/services
class ServiceLocator {
  static final TcpClient tcpClient = TcpClient();
  static late final Dispatcher dispatcher;
  
  static late final AuthService authService;
  static late final HistoryService historyService;
  static late final ProfileService profileService;
  static late final RoomService roomService;
  static late final GameService gameService;
  static late final GameStateService gameStateService;
  static late final FriendService friendService;
  static late final BonusService bonusService;

  static void init() {
    dispatcher = Dispatcher(tcpClient);
    authService = AuthService(tcpClient);
    historyService = HistoryService(tcpClient);
    profileService = ProfileService(tcpClient);
    roomService = RoomService(tcpClient, dispatcher);
    gameService = GameService(tcpClient, dispatcher);
    gameStateService = GameStateService(tcpClient, dispatcher);
    friendService = FriendService(tcpClient, dispatcher);
    bonusService = BonusService(tcpClient, dispatcher);
  }
}
