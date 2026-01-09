// UserCard.js
import { useState, useEffect } from "react";
import "./UserCard.css";
import { fetchProfile } from "../../services/profileService";

const DEFAULT_AVATAR_URL = "/bg/default-mushroom.jpg";

export default function UserCard() {
  const [profile, setProfile] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetchProfile()
      .then(data => {
        setProfile(data);
        setLoading(false);
      })
      .catch(err => {
        console.error("[UserCard] failed to fetch profile", err);
        setLoading(false);
      });
  }, []);

  const handleAvatarClick = () => {
    console.log("Open profile modal (later)");
  };

  if (loading) return <div className="user-card loading">...</div>;

  return (
    <div className="user-card">
      <div className="avatar" onClick={handleAvatarClick}>
        <img src={profile?.avatar || DEFAULT_AVATAR_URL} alt="Avatar" />
      </div>
      <div className="user-name">{profile?.name || "Anonymous"}</div>
    </div>
  );
}