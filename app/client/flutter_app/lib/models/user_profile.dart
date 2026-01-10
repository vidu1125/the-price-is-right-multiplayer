class UserProfile {
  final int id;
  final int accountId;
  String name;
  String avatar;
  String bio;
  final int matches;
  final int wins;
  final int points;
  final String badges;

  UserProfile({
    this.id = 0,
    this.accountId = 0,
    this.name = "",
    this.avatar = "",
    this.bio = "",
    this.matches = 0,
    this.wins = 0,
    this.points = 0,
    this.badges = "",
  });

  Map<String, dynamic> toJson() => {
    "id": id,
    "account_id": accountId,
    "name": name,
    "avatar": avatar,
    "bio": bio,
    "matches": matches,
    "wins": wins,
    "points": points,
    "badges": badges,
  };

  factory UserProfile.fromJson(Map<String, dynamic> json) {
    return UserProfile(
      id: json["id"] ?? 0,
      accountId: json["account_id"] ?? 0,
      name: json["name"] ?? "",
      avatar: json["avatar"] ?? "",
      bio: json["bio"] ?? "",
      matches: json["matches"] ?? 0,
      wins: json["wins"] ?? 0,
      points: json["points"] ?? 0,
      badges: json["badges"] ?? "",
    );
  }
}
