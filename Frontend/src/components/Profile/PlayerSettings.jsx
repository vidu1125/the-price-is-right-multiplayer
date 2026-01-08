import React, { useState, useEffect } from "react";
import { getAuthState } from "../../services/authService";
import { updateProfile } from "../../services/profileService";
import "./PlayerSettings.css";

export default function PlayerSettings() {
  const { profile } = getAuthState();
  const [name, setName] = useState(profile?.name || "");
  const [avatar, setAvatar] = useState(profile?.avatar || "");
  const [bio, setBio] = useState(profile?.bio || "");
  const [saving, setSaving] = useState(false);
  const [msg, setMsg] = useState("");
  const [error, setError] = useState("");
  const maxName = 50;

  const presetAvatars = [
    "/bg/default-mushroom.jpg",
    "/bg/crown.png"
  ];

  useEffect(() => {
    setMsg("");
  }, [name, avatar, bio]);

  const onSubmit = async (e) => {
    e.preventDefault();
    setSaving(true);
    setMsg("");
    setError("");
    if (name.trim().length === 0) {
      setError("Display name cannot be empty");
      setSaving(false);
      return;
    }
    if (avatar && !/^https?:\/\//.test(avatar)) {
      setError("Avatar must be a valid URL (http/https)");
      setSaving(false);
      return;
    }
    try {
      const res = await updateProfile({ name, avatar, bio });
      if (res?.success) {
        setMsg("Profile updated successfully!");
      } else {
        setMsg(res?.error || "An error occurred");
      }
    } catch (err) {
      setMsg(err?.error || "Could not update profile");
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="player-settings">
      <div className="settings-card">
        <div className="settings-header">
          <h2>Player Settings</h2>
          <p>Customize your profile so everyone recognizes you</p>
        </div>

        <div className="settings-grid">
          <form className="settings-form" onSubmit={onSubmit}>
            <label>
              <span>Display Name</span>
              <div className="input-with-info">
                <input
                  type="text"
                  value={name}
                  onChange={(e) => setName(e.target.value)}
                  placeholder="Enter your name"
                  maxLength={maxName}
                />
                <span className="char-count">{name.length}/{maxName}</span>
              </div>
            </label>

            <label>
              <span>Avatar (URL)</span>
              <input
                type="text"
                value={avatar}
                onChange={(e) => setAvatar(e.target.value)}
                placeholder="https://..."
              />
            </label>

            <div className="avatar-choices">
              {presetAvatars.map((src) => (
                <button
                  key={src}
                  type="button"
                  className={"avatar-choice" + (avatar === src ? " selected" : "")}
                  onClick={() => setAvatar(src)}
                >
                  <img src={src} alt="preset" />
                </button>
              ))}
            </div>

            <label>
              <span>Bio</span>
              <textarea
                value={bio}
                onChange={(e) => setBio(e.target.value)}
                placeholder="Brief description about you"
                rows={4}
              />
            </label>

            {error && <div className="error-msg">{error}</div>}
            {msg && <div className="status-msg">{msg}</div>}

            <button className="save-btn" type="submit" disabled={saving}>
              {saving ? "Saving..." : "Save Changes"}
            </button>
          </form>

          <div className="preview">
            <div className="avatar big">
              {avatar ? (
                <img src={avatar} alt="avatar" />
              ) : (
                <div className="placeholder">Ảnh</div>
              )}
            </div>
            <div className="info">
              <div className="name">{name || "(No name yet)"}</div>
              <div className="bio">{bio || "(No bio yet)"}</div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
