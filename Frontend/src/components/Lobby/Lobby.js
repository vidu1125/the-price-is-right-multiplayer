// Lobby.js (FIXED: UserCard nằm ngoài cột, đặt trực tiếp dưới .lobby)
import React, { useState, useEffect } from "react";
import "./Lobby.css";
import UserCard from "./UserCard";
import AppTitle from "./AppTitle";
import RoomPanel from "./RoomPanel";
import InvitationOverlay from "./InvitationOverlay";
import { useNavigate } from "react-router-dom";
import { logoutAccount } from "../../services/authService";
import { registerHandler, unregisterHandler } from "../../network/receiver";
import { OPCODE } from "../../network/opcode";
import { joinRoom } from "../../services/roomService";

export default function Lobby() {
  const navigate = useNavigate();
  const [activeInvitation, setActiveInvitation] = useState(null);

  useEffect(() => {
    const handleInvitation = (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const data = JSON.parse(text);
        console.log("[Lobby] Received invitation:", data);
        setActiveInvitation(data);
      } catch (e) {
        console.error("[Lobby] Failed to parse invitation:", e);
      }
    };

    registerHandler(OPCODE.NTF_INVITATION || 0x02C7, handleInvitation);

    return () => {
      unregisterHandler(OPCODE.NTF_INVITATION || 0x02C7, handleInvitation);
    };
  }, []);

  const handleLogout = async () => {
    try {
      await logoutAccount();
    } catch (err) {
      console.error("[Lobby] logout failed", err);
    }
    navigate("/login");
  };

  const onAcceptInvite = async () => {
    if (!activeInvitation) return;
    const roomId = activeInvitation.room_id;
    setActiveInvitation(null);

    try {
      console.log("[Lobby] Accepting invitation to room:", roomId);
      // Join room uses opcode CMD_JOIN_ROOM (0x0201)
      // The roomService handles navigation to /room on success
      await joinRoom(roomId);
    } catch (err) {
      alert("Failed to join room: " + err.message);
    }
  };

  const onDeclineInvite = () => {
    setActiveInvitation(null);
  };

  return (
    <div className="lobby">
      {/* Invitation Popup */}
      <InvitationOverlay
        invitation={activeInvitation}
        onAccept={onAcceptInvite}
        onDecline={onDeclineInvite}
      />

      {/* 1. USER CARD (Định vị tuyệt đối ở góc trên bên trái) */}
      <UserCard />

      {/* 2. APP TITLE (Nằm trên cùng, căn giữa) */}
      <AppTitle />

      {/* 3. LOBBY CONTENT (Khối 2 cột, căn giữa) */}
      <div className="lobby-content">
        <div className="lobby-left">
          <div className="side-actions">
            <button onClick={() => navigate("/history")}>
              VIEW HISTORY
            </button>

            <button onClick={() => navigate("/tutorial")}>
              VIEW TUTORIAL
            </button>

            <button onClick={() => navigate("/settings")}>SETTING</button>
            <button className="logout" onClick={handleLogout}>LOG OUT</button>
          </div>
        </div>

        <div className="lobby-right">
          <RoomPanel />
        </div>
      </div>
    </div>
  );
}