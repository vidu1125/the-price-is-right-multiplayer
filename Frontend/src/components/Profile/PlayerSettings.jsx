import React, { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import { getAuthState } from "../../services/authService";
import { updateProfile, changePassword } from "../../services/profileService";
import "./PlayerSettings.css";

export default function PlayerSettings() {
  const navigate = useNavigate();
  const { profile } = getAuthState();
  const [name, setName] = useState(profile?.name || "");
  const [avatar, setAvatar] = useState(profile?.avatar || "");
  const [bio, setBio] = useState(profile?.bio || "");
  const [saving, setSaving] = useState(false);
  const [msg, setMsg] = useState("");
  const [error, setError] = useState("");

  // Password Change State
  const [showPasswordForm, setShowPasswordForm] = useState(false);
  const [oldPassword, setOldPassword] = useState("");
  const [newPassword, setNewPassword] = useState("");
  const [passMsg, setPassMsg] = useState("");
  const [passError, setPassError] = useState("");
  const [changingPass, setChangingPass] = useState(false);

  const maxName = 50;

  const presetAvatars = [
    "/bg/default-mushroom.jpg",
    "/bg/crown.png",
    "/bg/duck.jpg",
    "/bg/frog.jpg",
    "/bg/ava3.jpg",
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
    // Removed manual Avatar URL validation logic

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

  const onPasswordSubmit = async (e) => {
    e.preventDefault();
    setChangingPass(true);
    setPassMsg("");
    setPassError("");

    try {
      const res = await changePassword(oldPassword, newPassword);
      if (res.success) {
        setPassMsg("Password changed successfully!");
        setOldPassword("");
        setNewPassword("");
        setTimeout(() => {
          setShowPasswordForm(false);
          setPassMsg("");
        }, 2000);
      } else {
        setPassError(res.error || "Failed to change password");
      }
    } catch (err) {
      setPassError(err.error || "Failed to change password");
    } finally {
      setChangingPass(false);
    }
  };

  return (
    <div className="player-settings">
      <div className="settings-card">
        <div className="settings-header">
          <div className="header-content">
            <h2>Player Settings</h2>
            <p>Customize your profile so everyone recognizes you</p>
          </div>
          <button
            className="back-btn"
            onClick={() => navigate("/")}
            title="Back to Lobby"
          >
            ← Back to Lobby
          </button>
        </div>

        <div className="settings-grid">
          <div className="settings-left-col">
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

              {/* Avatar URL input removed */}

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

            {/* Password Section */}
            <div className="password-section" style={{ marginTop: '2rem', borderTop: '1px solid #eee', paddingTop: '1rem' }}>
              <h3 style={{ marginBottom: '1rem', color: '#1F2A44' }}>Security</h3>
              {!showPasswordForm ? (
                <button
                  type="button"
                  className="change-pass-toggle-btn"
                  onClick={() => setShowPasswordForm(true)}
                  style={{
                    background: '#3B82F6', color: 'white', padding: '0.5rem 1rem',
                    border: 'none', borderRadius: '4px', cursor: 'pointer', fontWeight: 'bold'
                  }}
                >
                  Change Password
                </button>
              ) : (
                <form onSubmit={onPasswordSubmit} className="password-form-inline">
                  <div className="form-group" style={{ marginBottom: '0.5rem' }}>
                    <input
                      type="password"
                      placeholder="Current Password"
                      value={oldPassword}
                      onChange={(e) => setOldPassword(e.target.value)}
                      required
                      style={{ width: '100%', padding: '0.5rem', borderRadius: '4px', border: '1px solid #ccc' }}
                    />
                  </div>
                  <div className="form-group" style={{ marginBottom: '0.5rem' }}>
                    <input
                      type="password"
                      placeholder="New Password (min 6 chars)"
                      value={newPassword}
                      onChange={(e) => setNewPassword(e.target.value)}
                      required
                      minLength={6}
                      style={{ width: '100%', padding: '0.5rem', borderRadius: '4px', border: '1px solid #ccc' }}
                    />
                  </div>

                  {passError && <div style={{ color: 'red', fontSize: '0.9rem', marginBottom: '0.5rem' }}>{passError}</div>}
                  {passMsg && <div style={{ color: 'green', fontSize: '0.9rem', marginBottom: '0.5rem' }}>{passMsg}</div>}

                  <div style={{ display: 'flex', gap: '0.5rem' }}>
                    <button
                      type="submit"
                      disabled={changingPass}
                      style={{
                        background: '#10B981', color: 'white', padding: '0.5rem 1rem',
                        border: 'none', borderRadius: '4px', cursor: 'pointer', fontWeight: 'bold'
                      }}
                    >
                      {changingPass ? 'Updating...' : 'Update'}
                    </button>
                    <button
                      type="button"
                      onClick={() => setShowPasswordForm(false)}
                      style={{
                        background: '#6B7280', color: 'white', padding: '0.5rem 1rem',
                        border: 'none', borderRadius: '4px', cursor: 'pointer', fontWeight: 'bold'
                      }}
                    >
                      Cancel
                    </button>
                  </div>
                </form>
              )}
            </div>
          </div>

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
