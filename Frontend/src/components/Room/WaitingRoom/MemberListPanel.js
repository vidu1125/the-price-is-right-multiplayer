// MemberListPanel.js
import { useState } from "react";
import "./MemberListPanel.css";
import socketService from "../../../services/socketService";

const DEFAULT_AVATAR = "/bg/default-mushroom.jpg";
const CROWN_ICON = "/bg/crown.png"; 

export default function MemberListPanel({ isHost, roomId, roomName, roomCode, members, maxPlayers, onMemberKicked }) {
  const [kickingUserId, setKickingUserId] = useState(null);

  // Use provided members or fallback to mock data
  const memberList = members || [
    { id: 1, name: "HOST PLAYER", ready: true, isHost: true, avatar: null },
    { id: 2, name: "ALEX_MASTER", ready: true, isHost: false, avatar: null },
    { id: 3, name: "ALEX_MASTER", ready: true, isHost: false, avatar: null },
    { id: 4, name: "ALEX_MASTER", ready: false, isHost: false, avatar: null },
  ];

  const handleKick = async (memberId, memberName) => {
    if (!window.confirm(`Are you sure you want to kick ${memberName}?`)) {
      return;
    }

    setKickingUserId(memberId);
    try {
      const result = await socketService.kickMember(roomId, memberId);
      
      if (result.success) {
        alert(`${memberName} has been kicked`);
        
        // Callback to parent to refresh member list
        if (onMemberKicked) {
          onMemberKicked(memberId);
        }
      } else {
        alert(`Failed to kick member: ${result.error}`);
      }
    } catch (error) {
      alert("Error kicking member");
    } finally {
      setKickingUserId(null);
    }
  };

  const emptySlots = Array(Math.max(0, (maxPlayers || 6) - memberList.length)).fill(null);

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
        {memberList.map((member) => (
          <div key={member.id} className={`player-card ${member.isHost ? 'host-card' : ''}`}>
            {member.isHost ? (
              <img src={CROWN_ICON} alt="Crown" className="crown-image" />
            ) : (
              isHost && (
                <button 
                  className="card-kick-btn"
                  onClick={() => handleKick(member.id, member.name)}
                  disabled={kickingUserId === member.id}
                  title="Kick member"
                >
                  {kickingUserId === member.id ? '...' : '×'}
                </button>
              )
            )}
            
            <div className="card-content">
              <div className="avatar-circle-wrapper">
                <img src={member.avatar || DEFAULT_AVATAR} alt="avatar" className="avatar-circle" />
              </div>
              <span className="card-name">{member.name}</span>
              <div className={`status-tag ${member.ready ? 'ready' : 'waiting'}`}>
                {member.ready ? 'READY' : 'WAITING'}
              </div>
            </div>
          </div>
        ))}

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
        PLAYERS ({memberList.length}/{maxPlayers || 6})
      </div>
    </div>
  );
}