// WaitingRoom.js
import "./WaitingRoom.css";
import UserCard from "../../Lobby/UserCard"; 
import RoomTitle from "./RoomTitle";
import GameRulesPanel from "./GameRulesPanel"; 
import MemberListPanel from "./MemberListPanel";
import { useLocation, useNavigate } from "react-router-dom";
import { useState, useEffect } from "react";

export default function WaitingRoom() {
  const location = useLocation();
  const navigate = useNavigate();
  const [roomData, setRoomData] = useState(null);
  const [members, setMembers] = useState([]);
  const [loading, setLoading] = useState(true);
  
  // Game rules state
  const [gameRules, setGameRules] = useState({
    maxPlayers: 5,
    mode: "elimination",
    wagerMode: true,
    roundTime: "normal"
  });

  // Get room info from navigation state
  const { roomId, roomCode, isHost } = location.state || {};

  useEffect(() => {
    if (!roomId) {
      console.error("No room ID provided");
      navigate('/lobby');
      return;
    }

    // Fetch room details and members
    fetchRoomData(roomId);
  }, [roomId, navigate]);

  const fetchRoomData = async (id) => {
    try {
      const response = await fetch(`http://localhost:5000/api/room/${id}`);
      const data = await response.json();
      
      if (data.success) {
        setRoomData(data.room);
        setMembers(data.room.members || []);
        
        // Load game rules from room data
        setGameRules({
          maxPlayers: data.room.max_players || 5,
          visibility: data.room.visibility || "public",
          mode: data.room.mode || "scoring",
          wagerMode: data.room.wager_mode ?? false,
          roundTime: data.room.round_time || "normal"
        });
      } else {
        console.error("Failed to fetch room:", data.error);
        alert("Failed to load room: " + data.error);
        navigate('/lobby');
      }
    } catch (error) {
      console.error("Error fetching room:", error);
      alert("Error loading room");
      navigate('/lobby');
    } finally {
      setLoading(false);
    }
  };

  if (loading) {
    return <div className="waiting-room">Loading room...</div>;
  }

  if (!roomData) {
    return null;
  }

  const roomInfo = {
    id: roomId,
    name: roomData.name || "Room",
    code: roomCode || roomData.code || "N/A",
    isHost: isHost || false,
  };

  return (
    <div 
      className="waiting-room" 
      style={{ 
        backgroundImage: "url('/bg/waitingroom.png')",
        backgroundSize: '100% 100%', // Ép ảnh fit vừa khít màn hình
        backgroundPosition: 'center',
        backgroundRepeat: 'no-repeat',
        backgroundAttachment: 'fixed'
      }}
    >
      {/* Phần Header phía trên cùng */}
      <UserCard /> 
      <RoomTitle title="Waiting Room" /> 
      
      {/* Nội dung chính chia làm 3 cột */}
      <div className="waiting-room-content"> 
        
        {/* CỘT 1: GAME RULES (BÊN TRÁI) */}
        <div className="wr-left">
          <GameRulesPanel 
            isHost={roomInfo.isHost}
            roomId={roomInfo.id}
            gameRules={gameRules}
            onRulesChange={setGameRules}
          />
        </div>

        {/* CỘT 2: MEMBER LIST (Ở GIỮA) */}
        <div className="wr-center">
          <MemberListPanel 
            isHost={roomInfo.isHost} 
            roomId={roomInfo.id}
            hostId={roomData.host_id}
            roomName={roomInfo.name} 
            roomCode={roomInfo.code}
            maxPlayers={gameRules.maxPlayers}
            members={members}
            onRefresh={() => fetchRoomData(roomId)}
          />
        </div>

        {/* CỘT 3: CÁC NÚT HÀNH ĐỘNG (BÊN PHẢI) */}
        <div className="wr-right-actions">
          {roomInfo.isHost && (
            <button className="start-game-btn">START GAME</button>
          )}
          <button className="invite-btn">INVITE FRIENDS</button>
          <button className="leave-btn">LEAVE ROOM</button>
        </div>

      </div>
    </div>
  );
}