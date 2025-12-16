// RoomPanel.js
import "./RoomPanel.css";
import RoomList from "./RoomList";

export default function RoomPanel() {
  return (
    <div className="room-panel">
      <h3>Room List</h3>
      {/* THÊM CONTAINER CHO SCROLL */}
      <div className="room-list-container">
        <RoomList />
      </div>
      <div className="room-actions">
        <button onClick={() => console.log("Reload")}>Reload</button>
        <button onClick={() => console.log("Find room")}>Find room</button>
        <button className="create-room-btn" onClick={() => console.log("Create new room")}>
          + Create new room
        </button>
      </div>
    </div>
  );
}