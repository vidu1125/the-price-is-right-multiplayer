// WaitingRoom.js
import "./WaitingRoom.css";
import UserCard from "../../Lobby/UserCard"; 
import RoomTitle from "./RoomTitle";
import GameRulesPanel from "./GameRulesPanel"; 
import MemberListPanel from "./MemberListPanel";
import { useLocation, useNavigate } from "react-router-dom";
import { useState, useEffect } from "react";
// Import to register notification handlers AND access cached snapshots
import { getLatestRulesSnapshot, getLatestPlayersSnapshot, clearSnapshots, getRoomState } from "../../../services/hostService";
import { isConnected } from "../../../network/socketClient";

export default function WaitingRoom() {
  const location = useLocation();
  const navigate = useNavigate();
  const [roomData, setRoomData] = useState(null);
  const [members, setMembers] = useState([]);
  const [loading, setLoading] = useState(true);
  
  // Game rules state - ONLY updated by server notifications
  const [gameRules, setGameRules] = useState({
    maxPlayers: 5,
    mode: "scoring",
    wagerMode: false,
    visibility: "public"
  });

  // Get room info from sessionStorage (ONLY roomId, roomCode, isHost)
  const [roomId, setRoomId] = useState(null);
  const [roomCode, setRoomCode] = useState(null);
  const [isHost, setIsHost] = useState(false);

  useEffect(() => {
    // Load ONLY roomId, roomCode, isHost from localStorage
    const storedRoomId = localStorage.getItem('room_id');
    const storedRoomCode = localStorage.getItem('room_code');
    const storedIsHost = localStorage.getItem('is_host') === 'true';
    const storedRoomName = localStorage.getItem('room_name') || 'Room';
    
    if (!storedRoomId) {
      console.error("No room ID found");
      alert("Please create or join a room first.");
      navigate('/lobby');
      return;
    }

    setRoomId(parseInt(storedRoomId));
    setRoomCode(storedRoomCode);
    setIsHost(storedIsHost);
    
    // Set basic room data
    setRoomData({
      id: parseInt(storedRoomId),
      code: storedRoomCode,
      name: storedRoomName,
      host_id: 1 // TODO: Get from session
    });
    
    // Load cached snapshots (fix race condition)
    const cachedRules = getLatestRulesSnapshot();
    const cachedPlayers = getLatestPlayersSnapshot();
    
    if (cachedRules) {
      console.log('[WaitingRoom] Loading cached rules:', cachedRules);
      setGameRules(cachedRules);
    }
    
    if (cachedPlayers) {
      console.log('[WaitingRoom] Loading cached players:', cachedPlayers);
      setMembers(cachedPlayers);
    }
    
    // Clear snapshots after loading
    clearSnapshots();
    
    // Wait for socket connection before pulling snapshot
    const handleSocketConnected = () => {
      console.log('[WaitingRoom] Socket connected, pulling room state');
      getRoomState(parseInt(storedRoomId));
    };
    
    // If already connected, call immediately
    if (isConnected()) {
      getRoomState(parseInt(storedRoomId));
    } else {
      // Otherwise wait for connection event
      window.addEventListener('socket-connected', handleSocketConnected, { once: true });
    }
    
    setLoading(false);
    
    // Listen for server notifications
    const handleRulesChanged = (event) => {
      setGameRules(event.detail);
    };
    
    const handlePlayerList = (event) => {
      console.log('[WaitingRoom] Player list event:', event.detail);
      setMembers(event.detail);
    };
    
    const handlePlayerLeft = (event) => {
      console.log('[WaitingRoom] Player left event:', event.detail);
      // Will be updated via NTF_PLAYER_LIST
    };
    
    window.addEventListener('rules-changed', handleRulesChanged);
    window.addEventListener('player-list', handlePlayerList);
    window.addEventListener('player-left', handlePlayerLeft);
    
    // Cleanup
    return () => {
      window.removeEventListener('rules-changed', handleRulesChanged);
      window.removeEventListener('player-list', handlePlayerList);
      window.removeEventListener('player-left', handlePlayerLeft);
    };
  }, [navigate]);

  // No handleRulesChange - rules are ONLY updated by NTF_RULES_CHANGED

  if (loading) {
    return <div className="waiting-room">Loading room...</div>;
  }

  if (!roomData) {
    return null;
  }

  // No longer need roomInfo object

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
            isHost={isHost}
            roomId={roomId}
            gameRules={gameRules}
          />
        </div>

        {/* CỘT 2: MEMBER LIST (Ở GIỮA) */}
        <div className="wr-center">
          <MemberListPanel 
            isHost={isHost} 
            roomId={roomId}
            hostId={roomData?.host_id || 1}
            roomName={roomData?.name } 
            roomCode={roomCode}
            maxPlayers={gameRules.maxPlayers}
            members={members}
            onRefresh={() => console.log('[TODO] Refresh members')}
          />
        </div>

        {/* CỘT 3: CÁC NÚT HÀNH ĐỘNG (BÊN PHẢI) */}
        <div className="wr-right-actions">
          {isHost && (
            <button className="start-game-btn">START GAME</button>
          )}
          <button className="invite-btn">INVITE FRIENDS</button>
          <button className="leave-btn">LEAVE ROOM</button>
        </div>

      </div>
    </div>
  );
}