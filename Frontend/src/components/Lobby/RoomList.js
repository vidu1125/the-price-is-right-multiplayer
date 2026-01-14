// src/components/Lobby/RoomList.js
import "./RoomList.css";
import React, { useState, useEffect } from "react";
import { registerHandler } from "../../network/receiver";
import { sendPacket } from "../../network/dispatcher";
import { OPCODE } from "../../network/opcode";
import { joinRoom } from "../../services/roomService";

export default function RoomList() {
  const [rooms, setRooms] = useState([]);
  const [isLoading, setIsLoading] = useState(true);

  useEffect(() => {
    // Register handlers
    registerHandler(OPCODE.RES_ROOM_LIST, (payload) => {
      try {
        const text = new TextDecoder().decode(payload);
        const data = JSON.parse(text);
        console.log("[RoomList] Received rooms:", data);
        setRooms(data);
        setIsLoading(false);
      } catch (e) {
        console.error("[RoomList] Failed to parse room list:", e);
      }
    });

    // Handle empty or error cases
    registerHandler(OPCODE.ERR_SERVER_ERROR, (payload) => {
      console.error("[RoomList] Server error fetching rooms");
      setIsLoading(false);
    });

    // Send request immediately
    console.log("[RoomList] Fetching rooms...");
    sendPacket(OPCODE.CMD_GET_ROOM_LIST);

    // Refresh every 10 seconds? Optional, but good for lobby
    const interval = setInterval(() => {
      sendPacket(OPCODE.CMD_GET_ROOM_LIST);
    }, 10000);

    return () => clearInterval(interval);
  }, []);

  const handleJoin = async (roomId) => {
    console.log("Join room clicked:", roomId);
    try {
      await joinRoom(roomId, null); // Join by ID
    } catch (error) {
      console.error("Join room failed:", error);
    }
  };

  return (
    <div className="room-table-container">
      <table className="room-table">
        <thead>
          <tr>
            {/* <th>ID</th> -- User requested to remove ID */}
            <th>Name</th>
            <th>Mode</th>
            <th>Players</th>
            <th>Status</th>
            <th></th>
          </tr>
        </thead>
        <tbody>
          {isLoading && rooms.length === 0 && (
            <tr><td colSpan="5" style={{ textAlign: "center" }}>Loading rooms...</td></tr>
          )}

          {!isLoading && rooms.length === 0 && (
            <tr><td colSpan="5" style={{ textAlign: "center" }}>No waiting rooms found.</td></tr>
          )}

          {rooms.map((r) => {
            const isPrivate = r.visibility === "private";
            return (
              <tr key={r.id}>
                {/* <td>{r.id}</td> */}
                <td>{r.name}</td>
                <td>{r.mode === 0 || r.mode === "elimination" ? "Elimination" : "Scoring"}</td>
                <td>
                  {/* Display Current / Max */}
                  {r.current_players !== undefined
                    ? `${r.current_players} / ${r.max_players}`
                    : `? / ${r.max_players}`}
                </td>
                <td>
                  {isPrivate ? (
                    <span style={{ color: "red", fontWeight: "bold" }}>Locked 🔒</span>
                  ) : (
                    r.status === "waiting" ? "Waiting" : r.status
                  )}
                </td>
                <td>
                  {!isPrivate ? (
                    <button
                      onClick={() => handleJoin(r.id)}
                      disabled={r.status !== "waiting"}
                      className="join-btn"
                    >
                      Join
                    </button>
                  ) : (
                    <span style={{ color: "#888", fontSize: "0.9em" }}>Invite Only</span>
                  )}
                </td>
              </tr>
            );
          })}
        </tbody>
      </table>
    </div>
  );
}