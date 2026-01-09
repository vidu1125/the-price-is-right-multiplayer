import '../network/tcp_client.dart';
import '../network/dispatcher.dart';
import 'auth_service.dart';
import 'history_service.dart';
import 'profile_service.dart';

/// Centralized access to all singletons/services
class ServiceLocator {
  static final TcpClient tcpClient = TcpClient();
  static late final Dispatcher dispatcher;
  
  static late final AuthService authService;
  static late final HistoryService historyService;
  static late final ProfileService profileService;

  static void init() {
    dispatcher = Dispatcher(tcpClient);
    authService = AuthService(tcpClient);
    historyService = HistoryService(tcpClient);
    profileService = ProfileService();
  }
}
