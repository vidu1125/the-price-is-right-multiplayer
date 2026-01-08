// WaitingRoom.js
import "./WaitingRoom.css";
import UserCard from "../../Lobby/UserCard"; 
import RoomTitle from "./RoomTitle";
import GameRulesPanel from "./GameRulesPanel"; 
import MemberListPanel from "./MemberListPanel";
import { useLocation, useNavigate } from "react-router-dom";
import { useState, useEffect } from "react";
// Import to register notification handlers AND access cached snapshots
import { getLatestRulesSnapshot, getLatestPlayersSnapshot, clearSnapshots, getRoomState, startGame } from "../../../services/hostService";
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
  
  // START GAME modal states
  const [showStartModal, setShowStartModal] = useState(false);
  const [errorMessage, setErrorMessage] = useState('');

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

  // No handleRulesChange - rules are ONLY updated by NTF_RULES_CHANGED
  
  const handleStartGame = () => {
    if (roomId) {
      startGame(roomId);
    }
  };

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
            <button className="start-game-btn" onClick={handleStartGame}>
              START GAME
            </button>
          )}
          <button className="invite-btn">INVITE FRIENDS</button>
          <button className="leave-btn">LEAVE ROOM</button>
        </div>

      </div>
      
      {/* Error Modal */}
      {errorMessage && (
        <div style={{
          position: 'fixed',
          top: 0,
          left: 0,
          right: 0,
          bottom: 0,
          background: 'rgba(0, 0, 0, 0.85)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          zIndex: 9999
        }}>
          <div style={{
            background: '#1a1a2e',
            border: '3px solid #ff6b6b',
            borderRadius: '20px',
            padding: '50px',
            textAlign: 'center',
            minWidth: '450px',
            boxShadow: '0 20px 60px rgba(255, 107, 107, 0.3)'
          }}>
            <h2 style={{ 
              color: '#ff6b6b', 
              fontSize: '2.5em',
              marginBottom: '20px',
              fontWeight: 'bold'
            }}>
              ❌ Cannot Start Game
            </h2>
            <p style={{ 
              fontSize: '1.3em',
              color: '#fff',
              marginBottom: '30px',
              lineHeight: '1.6'
            }}>
              {errorMessage}
            </p>
            <button 
              onClick={() => setErrorMessage('')}
              style={{
                padding: '12px 40px',
                fontSize: '1.1em',
                background: '#ff6b6b',
                border: 'none',
                borderRadius: '10px',
                color: 'white',
                cursor: 'pointer',
                fontWeight: 'bold'
              }}
            >
              OK
            </button>
          </div>
        </div>
      )}
      
      {/* Success Modal */}
      {showStartModal && (
        <div style={{
          position: 'fixed',
          top: 0,
          left: 0,
          right: 0,
          bottom: 0,
          background: 'rgba(0, 0, 0, 0.9)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          zIndex: 9999
        }}>
          <div style={{
            background: 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
            border: '3px solid #4ecdc4',
            borderRadius: '20px',
            padding: '60px',
            textAlign: 'center',
            minWidth: '500px',
            animation: 'pulse 1s infinite'
          }}>
            <h1 style={{ 
              color: '#fff', 
              fontSize: '4em',
              marginBottom: '20px',
              textShadow: '0 0 20px rgba(255,255,255,0.5)'
            }}>
              🎮 Game Starting!
            </h1>
            <p style={{ 
              fontSize: '1.5em',
              color: '#4ecdc4',
              fontWeight: 'bold'
            }}>
              Get Ready...
            </p>
          </div>
        </div>
      )}
    </div>
  );
}