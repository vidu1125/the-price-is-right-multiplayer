class UserProfile {
  String name;
  String avatar;
  String bio;

  UserProfile({
    this.name = "",
    this.avatar = "",
    this.bio = "",
  });

  Map<String, dynamic> toJson() => {
    "name": name,
    "avatar": avatar,
    "bio": bio,
  };

  factory UserProfile.fromJson(Map<String, dynamic> json) {
    return UserProfile(
      name: json["name"] ?? "",
      avatar: json["avatar"] ?? "",
      bio: json["bio"] ?? "",
    );
  }
}
