import { useState } from "react";
import "./CreateRoomPanel.css";
import socketService from "../../services/socketService";

export default function CreateRoomPanel({ onRoomCreated }) {
  const [showModal, setShowModal] = useState(false);
  const [loading, setLoading] = useState(false);
  const [formData, setFormData] = useState({
    name: "",
    visibility: "public",
    mode: "scoring",
    maxPlayers: 4,
    roundTime: 30,
    wagerEnabled: false
  });

  const handleCreate = async () => {
    if (!formData.name.trim()) {
      alert("Please enter room name");
      return;
    }

    setLoading(true);
    try {
      const result = await socketService.createRoom(formData);
      
      if (result.success) {
        alert(`Room created! Code: ${result.room.code}`);
        setShowModal(false);
        setFormData({
          name: "",
          visibility: "public",
          mode: "scoring",
          maxPlayers: 4,
          roundTime: 30,
          wagerEnabled: false
        });
        
        // Callback to parent component
        if (onRoomCreated) {
          onRoomCreated(result.room);
        }
      } else {
        alert(`Failed: ${result.error}`);
      }
    } catch (error) {
      alert("Error creating room");
    } finally {
      setLoading(false);
    }
  };

  return (
    <>
      <div className="create-room-panel">
        <h3>Create room</h3>
        <button onClick={() => setShowModal(true)}>+ Create new room</button>
      </div>

      {showModal && (
        <div className="modal-overlay" onClick={() => setShowModal(false)}>
          <div className="modal-content" onClick={(e) => e.stopPropagation()}>
            <h2>Create New Room</h2>
            
            <div className="form-group">
              <label>Room Name:</label>
              <input 
                type="text" 
                value={formData.name}
                onChange={(e) => setFormData({...formData, name: e.target.value})}
                placeholder="Enter room name"
                maxLength={63}
              />
            </div>

            <div className="form-group">
              <label>Visibility:</label>
              <select 
                value={formData.visibility}
                onChange={(e) => setFormData({...formData, visibility: e.target.value})}
              >
                <option value="public">Public</option>
                <option value="private">Private</option>
              </select>
            </div>

            <div className="form-group">
              <label>Mode:</label>
              <select 
                value={formData.mode}
                onChange={(e) => setFormData({...formData, mode: e.target.value})}
              >
                <option value="scoring">Scoring</option>
                <option value="elimination">Elimination</option>
              </select>
            </div>

            <div className="form-group">
              <label>Max Players:</label>
              <select 
                value={formData.maxPlayers}
                onChange={(e) => setFormData({...formData, maxPlayers: parseInt(e.target.value)})}
              >
                <option value={4}>4</option>
                <option value={5}>5</option>
                <option value={6}>6</option>
              </select>
            </div>

            <div className="form-group">
              <label>Round Time (seconds):</label>
              <input 
                type="number" 
                min={15} 
                max={60}
                value={formData.roundTime}
                onChange={(e) => setFormData({...formData, roundTime: parseInt(e.target.value)})}
              />
            </div>

            <div className="form-group">
              <label>
                <input 
                  type="checkbox"
                  checked={formData.wagerEnabled}
                  onChange={(e) => setFormData({...formData, wagerEnabled: e.target.checked})}
                />
                Enable Wager Mode
              </label>
            </div>

            <div className="modal-actions">
              <button onClick={handleCreate} disabled={loading}>
                {loading ? "Creating..." : "Create Room"}
              </button>
              <button onClick={() => setShowModal(false)}>Cancel</button>
            </div>
          </div>
        </div>
      )}
    </>
  );
}
