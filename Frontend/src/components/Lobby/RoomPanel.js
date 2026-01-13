// RoomPanel.js
import "./RoomPanel.css";
import RoomList from "./RoomList";
import { createRoom } from "../../services/hostService";
import JoinByCodeModal from "./JoinByCodeModal";
import React, { useState, useEffect, useRef } from "react";
import { registerHandler } from "../../network/receiver";
import { OPCODE } from "../../network/opcode";
import { useNavigate } from "react-router-dom";

export default function RoomPanel() {
  const navigate = useNavigate();
  const [showModal, setShowModal] = useState(false);
  const [showFindModal, setShowFindModal] = useState(false);

  // Khởi tạo state form
  const [formData, setFormData] = useState({
    name: "My Room",
    visibility: "public",
    mode: "scoring",
    maxPlayers: 6,
    roundTime: 15,
    wagerEnabled: false
  });

  // Xử lý khi đổi Mode
  const handleModeChange = (newMode) => {
    let newMax = formData.maxPlayers;

    if (newMode === "elimination") {
      newMax = 4; // elimination cố định 4 người
    } else if (newMode === "scoring") {
      // Nếu chuyển sang Scoring, đảm bảo số người trong khoảng 4-6
      if (newMax < 4) newMax = 4;
      if (newMax > 6) newMax = 6;
    }

    setFormData({
      ...formData,
      mode: newMode,
      maxPlayers: newMax
    });
  };

  // Use ref to keep track of latest formData without re-binding event listener
  const formDataRef = useRef(formData);

  useEffect(() => {
    formDataRef.current = formData;
  }, [formData]);

  useEffect(() => {
    // Handler for RES_ROOM_CREATED moved to hostService.js to handle binary payload correctly
    // and centralised logic.
    /*
    registerHandler(OPCODE.RES_ROOM_CREATED, (payload) => { ... }); 
    */

    // Listen for room creation success from hostService.js
    const onRoomCreated = (e) => {
      const { roomId, roomCode } = e.detail;
      console.log("🚀 Event received: room_created", e.detail);
      console.log("👉 Current Form Data:", formDataRef.current); // Debug log

      setShowModal(false);
      navigate('/waitingroom', {
        state: {
          roomId,
          roomCode,
          roomName: formDataRef.current.name, // Pass room name from ref
          isHost: true,
          gameRules: {
            maxPlayers: formDataRef.current.maxPlayers,
            mode: formDataRef.current.mode,
            wagerMode: formDataRef.current.wagerEnabled,
            roundTime: formDataRef.current.roundTime || 15,
            visibility: formDataRef.current.visibility || "public"
          }
        }
      });
    };
    window.addEventListener('room_created', onRoomCreated);

    // Xử lý khi Join thành công (từ roomService)
    const onRoomJoined = (e) => {
      console.log("🚀 Event received: room_joined", e.detail);
      const roomData = e.detail;

      navigate('/waitingroom', {
        state: {
          roomId: roomData.roomId,
          roomCode: roomData.roomCode,
          roomName: roomData.roomName,
          hostId: roomData.hostId,
          isHost: roomData.isHost,
          gameRules: roomData.gameRules,
          players: roomData.players // Pass initial player list
        }
      });
    };
    window.addEventListener('room_joined', onRoomJoined);

    registerHandler(OPCODE.ERR_BAD_REQUEST, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const data = JSON.parse(text);
        alert(`❌ Error: ${data.error || text}`);
      } catch (err) {
        alert(`❌ Error: ${text}`);
      }
    });

    return () => {
      window.removeEventListener('room_created', onRoomCreated);
      window.removeEventListener('room_joined', onRoomJoined);
      // unregisterHandler(OPCODE.RES_ROOM_CREATED);
    };
  }, []); // Only run once on mount

  const handleCreateRoom = () => {
    createRoom(formData);
  };

  return (
    <div className="room-panel">
      <h3>Room List</h3>
      <div className="room-list-container">
        <RoomList />
      </div>
      <div className="room-actions">
        <button onClick={() => console.log("Reload")}>Reload</button>
        <button onClick={() => setShowFindModal(true)}>Find room</button>
        <button className="create-room-btn" onClick={() => setShowModal(true)}>
          + Create new room
        </button>
      </div>

      {/* CREATE ROOM MODAL */}
      {showModal && (
        <div className="modal-overlay" onClick={() => setShowModal(false)}>
          <div className="modal-content" onClick={(e) => e.stopPropagation()}>
            <h2>Create New Room</h2>
            <form onSubmit={(e) => { e.preventDefault(); handleCreateRoom(); }}>

              <label>
                Room Name:
                <input
                  type="text"
                  value={formData.name}
                  onChange={(e) => setFormData({ ...formData, name: e.target.value })}
                  maxLength={63}
                  required
                />
              </label>

              <label>
                Visibility:
                <select value={formData.visibility} onChange={(e) => setFormData({ ...formData, visibility: e.target.value })}>
                  <option value="public">Public</option>
                  <option value="private">Private</option>
                </select>
              </label>

              <label>
                Game Mode:
                <select value={formData.mode} onChange={(e) => handleModeChange(e.target.value)}>
                  <option value="scoring">Scoring</option>
                  <option value="elimination">Elimination</option>
                </select>
              </label>

              {/* --- PHẦN MAX PLAYERS (Đã sửa lại giống Round Time) --- */}
              <label>
                Max Players:
                <input
                  type="number"
                  value={formData.maxPlayers}
                  onChange={(e) => setFormData({ ...formData, maxPlayers: parseInt(e.target.value) })}
                  min={4}
                  max={formData.mode === "elimination" ? 4 : 6}
                  disabled={formData.mode === "elimination"} /* Khóa input nếu là elimination */
                  required
                />
              </label>
              {/* --------------------------------------------------- */}



              <label className="checkbox-label">
                <input
                  type="checkbox"
                  checked={formData.wagerEnabled}
                  onChange={(e) => setFormData({ ...formData, wagerEnabled: e.target.checked })}
                />
                Enable Wager Mode
              </label>

              <div className="modal-actions">
                <button type="submit" className="btn-primary">Create Room</button>
                <button type="button" onClick={() => setShowModal(false)} className="btn-secondary">Cancel</button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* FIND ROOM MODAL */}
      {showFindModal && (
        <JoinByCodeModal onClose={() => setShowFindModal(false)} />
      )}
    </div>
  );
}