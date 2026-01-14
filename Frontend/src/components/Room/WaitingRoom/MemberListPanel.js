// MemberListPanel.js
import "./MemberListPanel.css";
import { kickMember } from "../../../services/hostService";
import { toggleReady } from "../../../services/roomService";
import { registerHandler } from "../../../network/receiver";
import { OPCODE } from "../../../network/opcode";
import { useEffect } from "react";

const DEFAULT_AVATAR = "/bg/default-mushroom.jpg";
const CROWN_ICON = "/bg/crown.png";

export default function MemberListPanel({ isHost, roomId, hostId, roomName, roomCode, maxPlayers = 6, members = [], onRefresh }) {
  const emptySlots = Array(Math.max(0, maxPlayers - members.length)).fill(null);

  // Get current user ID from localStorage
  const profile = JSON.parse(localStorage.getItem('profile') || '{}');
  const currentUserId = profile.account_id;

  useEffect(() => {
    // Register handler for kick response
    registerHandler(OPCODE.RES_MEMBER_KICKED, (payload) => {
      console.log("✅ Member kicked response");
      const text = new TextDecoder().decode(payload);
      console.log("Response:", text);

      // Refresh member list
      if (onRefresh) {
        onRefresh();
      }
    });
  }, [onRefresh]);

  const handleKick = (memberId) => {
    if (!window.confirm("Are you sure you want to kick this member?")) {
      return;
    }
    console.log("🔵 Kicking member:", memberId, "from room:", roomId);
    kickMember(roomId, memberId);
  };

  return (
    <div className="member-list-wrapper">
      {/* Room ID được đặt tuyệt đối ở góc để không đẩy dòng tiêu đề xuống */}
      <div className="room-id-tag">
        ID: {roomCode || "XYZ456"}
      </div>

      <div className="member-list-header">
        <h2 className="room-name-display">
          {roomName || "Fun room"}
        </h2>
      </div>

      <div className="player-grid">
        {members.map((member) => {
          if (!member) return null; // Safe check
          const isHostMember = member.account_id === hostId;
          const isReady = member.is_ready || member.ready; // Handle both cases

          return (
            <div key={member.account_id} className={`player-card ${isHostMember ? 'host-card' : ''}`}>
              {isHostMember ? (
                <img src={CROWN_ICON} alt="Crown" className="crown-image" />
              ) : (
                isHost && <button className="card-kick-btn" onClick={() => handleKick(member.account_id)}>×</button>
              )}

              <div className="card-content">
                <div className="avatar-circle-wrapper">
                  <img src={member.avatar || DEFAULT_AVATAR} alt="avatar" className="avatar-circle" />
                </div>
                <span className="card-name">{member.name || `Player ${member.account_id}`}</span>
                <div className={`status-tag ${isReady ? 'ready' : 'waiting'}`}>
                  {isReady ? 'READY' : 'WAITING'}
                </div>
              </div>
            </div>
          );
        })}

        {emptySlots.map((_, index) => (
          <div key={`empty-${index}`} className={`player-card empty`}>
            <div className="card-content">
              <div className="avatar-circle-wrapper empty-ava">?</div>
              <span className="card-name">EMPTY</span>
            </div>
          </div>
        ))}
      </div>

      <div className="grid-footer">
        PLAYERS ({members.length}/{maxPlayers})
      </div>
    </div>
  );
}