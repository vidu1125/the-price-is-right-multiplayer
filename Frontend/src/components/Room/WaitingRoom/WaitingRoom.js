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
    mode: "eliminate",
    wagerMode: true,
    roundTime: "normal"
  });

  // Get room info from sessionStorage (ONLY roomId, roomCode, isHost)
  const [roomId, setRoomId] = useState(null);
  const [roomCode, setRoomCode] = useState(null);
  const [isHost, setIsHost] = useState(false);
  const [hostId, setHostId] = useState(null);
  
  // START GAME modal states
  const [showStartModal, setShowStartModal] = useState(false);
  const [errorMessage, setErrorMessage] = useState('');

  useEffect(() => {
    if (!roomId) {
      console.error("No room ID provided");
      navigate('/lobby');
      return;
    }

    setRoomId(parseInt(storedRoomId));
    setRoomCode(storedRoomCode);
    setIsHost(storedIsHost);
    
    // Get host_id from localStorage (set by room state response)
    const storedHostId = parseInt(localStorage.getItem('host_id') || '0');
    setHostId(storedHostId);
    
    // Set basic room data
    setRoomData({
      id: parseInt(storedRoomId),
      code: storedRoomCode,
      name: storedRoomName,
      host_id: storedHostId
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
      console.log('[WaitingRoom] Rules changed event received:', event.detail);
      setGameRules(event.detail);
    };
    
    const handlePlayerList = (event) => {
      console.log('[WaitingRoom] Player list event:', event.detail);
      // Normalize players: remove duplicates by account_id
      const normalizedPlayers = Object.values(
        (event.detail || []).reduce((acc, player) => {
          acc[player.account_id] = player; // Keep last occurrence
          return acc;
        }, {})
      );
      
      // Update host_id from player list
      const hostPlayer = normalizedPlayers.find(p => p.is_host);
      if (hostPlayer) {
        setHostId(hostPlayer.account_id);
        setRoomData(prev => ({ ...prev, host_id: hostPlayer.account_id }));
      }
      
      console.log('[WaitingRoom] Normalized players:', normalizedPlayers);
      setMembers(normalizedPlayers); // REPLACE state, not append
    };
    
    const handlePlayerLeft = (event) => {
      console.log('[WaitingRoom] Player left event:', event.detail);
      // Will be updated via NTF_PLAYER_LIST
    };
    
    // Listen for START GAME events
    const handleGameStarting = (e) => {
      console.log('[WaitingRoom] Game starting!', e.detail);
      setShowStartModal(true);
      
      // Navigate to game after 3 seconds
      setTimeout(() => {
        window.location.href = '/game';
      }, 3000);
    };
    
    const handleStartError = (e) => {
      console.error('[WaitingRoom] Start game error:', e.detail.message);
      setErrorMessage(e.detail.message);
      
      // Auto hide after 5 seconds
      setTimeout(() => setErrorMessage(''), 5000);
    };
    
    window.addEventListener('rules-changed', handleRulesChanged);
    window.addEventListener('player-list', handlePlayerList);
    window.addEventListener('player-left', handlePlayerLeft);
    window.addEventListener('game-starting', handleGameStarting);
    window.addEventListener('game-start-error', handleStartError);
    
    // Cleanup
    return () => {
      window.removeEventListener('rules-changed', handleRulesChanged);
      window.removeEventListener('player-list', handlePlayerList);
      window.removeEventListener('player-left', handlePlayerLeft);
      window.removeEventListener('game-starting', handleGameStarting);
      window.removeEventListener('game-start-error', handleStartError);
    };
  }, []); // ✅ FIX: Empty dependency - only run once on mount

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
            isHost={isHost} 
            roomId={roomId}
            hostId={hostId}
            roomName={roomData?.name } 
            roomCode={roomCode}
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