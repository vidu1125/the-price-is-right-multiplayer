// RoomPanel.js
import "./RoomPanel.css";
import RoomList from "./RoomList";
import { createRoom } from "../../services/hostService";
import { useState, useEffect } from "react";
import { registerHandler } from "../../network/receiver";
import { OPCODE } from "../../network/opcode";
import { useNavigate } from "react-router-dom";

export default function RoomPanel() {
  const navigate = useNavigate();
  const [showModal, setShowModal] = useState(false);

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

    if (newMode === "eliminate") {
      newMax = 4; // Eliminate cố định 4 người
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

  useEffect(() => {
    registerHandler(OPCODE.RES_ROOM_CREATED, (payload) => {
      console.log("✅ Room created response received");
      try {
        const data = JSON.parse(new TextDecoder().decode(payload));
        const roomCode = data.room_code || data.room?.code || data.code;
        const roomId = data.room_id || data.room?.id;
        const roomName = data.room_name || data?.room_name;

        // Lưu vào localStorage
        localStorage.setItem('room_id', String(roomId));
        localStorage.setItem('room_code', roomCode);
        localStorage.setItem('room_name', roomName);
        localStorage.setItem('is_host', 'true');

        console.log('[RoomPanel] Stored to localStorage:', { roomId, roomCode, roomName });

        setShowModal(false);
        
        // Use React Router navigate (NO page reload, keeps WebSocket alive)
        navigate('/waitingroom');
      } catch (err) {
        console.error("Parse error:", err);
      }
    });

    registerHandler(OPCODE.ERR_BAD_REQUEST, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const data = JSON.parse(text);
        alert(`❌ Error: ${data.error || text}`);
      } catch (err) {
        alert(`❌ Error: ${text}`);
      }
    });
  }, [navigate]);

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
        <button onClick={() => console.log("Find room")}>Find room</button>
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
                  <option value="eliminate">Eliminate</option>
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
                  max={formData.mode === "eliminate" ? 4 : 6}
                  disabled={formData.mode === "eliminate"} /* Khóa input nếu là Eliminate */
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
    </div>
  );
}