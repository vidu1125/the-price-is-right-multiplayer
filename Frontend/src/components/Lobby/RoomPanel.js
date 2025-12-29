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
  const [formData, setFormData] = useState({
    name: "My Room",
    visibility: "public",
    mode: "normal",
    maxPlayers: 6,
    roundTime: 15,
    wagerEnabled: false
  });

  useEffect(() => {
    // Register handler for room created response
    registerHandler(OPCODE.RES_ROOM_CREATED, (payload) => {
      console.log("✅ Room created response received");
      const text = new TextDecoder().decode(payload);
      console.log("Response:", text);
      try {
        // Parse JSON from response
        let jsonStr = text;
        const bodyStart = text.indexOf('\r\n\r\n');
        if (bodyStart !== -1) {
          jsonStr = text.substring(bodyStart + 4);
        }
        const data = JSON.parse(jsonStr);
        const roomCode = data.room_code || data.room?.code || data.code;
        const roomId = data.room_id || data.room?.id;
        
        console.log("🎉 Navigating to room:", roomId, roomCode);
        setShowModal(false);
        
        // Navigate to waiting room with room info
        navigate('/waitingroom', { 
          state: { 
            roomId: roomId,
            roomCode: roomCode,
            isHost: true 
          } 
        });
      } catch (err) {
        console.error("Parse error:", err);
        alert("Room created but failed to parse response");
      }
    });

    // Register handler for error responses
    registerHandler(OPCODE.ERR_BAD_REQUEST, (payload) => {
      console.error("❌ Bad request error");
      const text = new TextDecoder().decode(payload);
      console.error("Error response:", text);
      try {
        const data = JSON.parse(text);
        alert(`❌ Error: ${data.error || text}`);
      } catch (err) {
        alert(`❌ Error: ${text}`);
      }
    });
  }, [navigate]);

  const handleCreateRoom = () => {
    console.log("🔵 CREATE ROOM button clicked");
    console.log("📤 Sending createRoom request:", formData);
    
    createRoom(formData);  // Just send, response will come via handler
    // Don't close modal yet, wait for response
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
                  onChange={(e) => setFormData({...formData, name: e.target.value})}
                  maxLength={63}
                  required
                />
              </label>

              <label>
                Visibility:
                <select value={formData.visibility} onChange={(e) => setFormData({...formData, visibility: e.target.value})}>
                  <option value="public">Public</option>
                  <option value="private">Private</option>
                </select>
              </label>

              <label>
                Game Mode:
                <select value={formData.mode} onChange={(e) => setFormData({...formData, mode: e.target.value})}>
                  <option value="normal">Normal</option>
                  <option value="elimination">Elimination</option>
                </select>
              </label>

              <label>
                Max Players:
                <input 
                  type="number" 
                  value={formData.maxPlayers} 
                  onChange={(e) => setFormData({...formData, maxPlayers: parseInt(e.target.value)})}
                  min={2}
                  max={8}
                  required
                />
              </label>

              <label>
                Round Time (seconds):
                <input 
                  type="number" 
                  value={formData.roundTime} 
                  onChange={(e) => setFormData({...formData, roundTime: parseInt(e.target.value)})}
                  min={10}
                  max={60}
                  required
                />
              </label>

              <label>
                <input 
                  type="checkbox" 
                  checked={formData.wagerEnabled} 
                  onChange={(e) => setFormData({...formData, wagerEnabled: e.target.checked})}
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