class RoomMember {
  final int accountId;
  final String email;
  final String? avatar;
  final bool ready;

  RoomMember({
    required this.accountId,
    required this.email,
    this.avatar,
    this.ready = false,
  });

  factory RoomMember.fromJson(Map<String, dynamic> json) {
    return RoomMember(
      accountId: json['account_id'] as int,
      email: json['email'] as String? ?? "Player ${json['account_id']}",
      avatar: json['avatar'] as String?,
      ready: json['ready'] == true, // Handle boolean or missing
    );
  }
}

class Room {
  final int id;
  final String name;
  final String? code;
  final int hostId;
  final int maxPlayers;
  final String visibility;
  final String mode;
  final bool wagerMode;
  final int roundTime; // in seconds, or maybe enum?
  final String status;
  final List<RoomMember> members;
  final int currentPlayerCount;

  Room({
    required this.id,
    required this.name,
    this.code,
    required this.hostId,
    required this.maxPlayers,
    required this.visibility,
    required this.mode,
    required this.wagerMode,
    required this.roundTime,
    required this.members,
    this.status = "waiting",
    this.currentPlayerCount = 0,
  });

  factory Room.fromJson(Map<String, dynamic> json) {
    var memberList = <RoomMember>[];
    if (json['members'] != null) {
      memberList = (json['members'] as List)
          .map((m) => RoomMember.fromJson(m))
          .toList();
    }

    int count = json['current_players'] is int ? json['current_players'] : memberList.length;

    return Room(
      id: json['id'] as int,
      name: json['name'] as String? ?? "Room",
      code: json['code'] as String?,
      hostId: json['host_id'] as int? ?? 0,
      maxPlayers: json['max_players'] as int? ?? 6,
      visibility: json['visibility'] as String? ?? "public",
      mode: json['mode'] as String? ?? "scoring",
      wagerMode: json['wager_mode'] == true, 
      roundTime: _parseRoundTime(json['round_time']),
      members: memberList,
      status: json['status'] as String? ?? "waiting",
      currentPlayerCount: count,
    );
  }

  static int _parseRoundTime(dynamic val) {
    if (val is int) return val;
    if (val == "normal") return 15;
    if (val == "fast") return 10;
    if (val == "slow") return 20;
    return 15;
  }
}
