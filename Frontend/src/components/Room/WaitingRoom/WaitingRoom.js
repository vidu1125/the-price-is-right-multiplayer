// WaitingRoom.js
import { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import "./WaitingRoom.css";
import UserCard from "../../Lobby/UserCard"; 
import RoomTitle from "./RoomTitle";
import GameRulesPanel from "./GameRulesPanel"; 
import MemberListPanel from "./MemberListPanel";
import socketService from "../../../services/socketService";

export default function WaitingRoom({ roomId, role }) { 
  const navigate = useNavigate();
  const [roomInfo, setRoomInfo] = useState({
    name: "The Price is Right (A)",
    code: "XYZ456",
    isHost: true,
  });
  const [members, setMembers] = useState([]);
  const [rules, setRules] = useState({
    maxPlayers: 6,
    mode: 'elimination',
    wagerEnabled: false,
    roundTime: 30
  });
  const [startingGame, setStartingGame] = useState(false);
  const [countdown, setCountdown] = useState(null);

  // Load room data on mount
  useEffect(() => {
    // TODO: Fetch room data from backend
    // For now using mock data
  }, [roomId]);

  const handleStartGame = async () => {
    if (!window.confirm("Start the game now? All players should be ready.")) {
      return;
    }

    setStartingGame(true);
    try {
      const result = await socketService.startGame(roomId || 1);
      
      if (result.success) {
        // Calculate countdown
        const countdownMs = result.countdown || 3000;
        const countdownSeconds = Math.ceil(countdownMs / 1000);
        
        setCountdown(countdownSeconds);
        
        // Countdown timer
        const interval = setInterval(() => {
          setCountdown(prev => {
            if (prev <= 1) {
              clearInterval(interval);
              // Navigate to game screen
              navigate(`/game/${result.matchId}`);
              return 0;
            }
            return prev - 1;
          });
        }, 1000);
        
        alert(`Game starting in ${countdownSeconds} seconds!`);
      } else {
        alert(`Failed to start game: ${result.error}`);
        setStartingGame(false);
      }
    } catch (error) {
      alert("Error starting game");
      setStartingGame(false);
    }
  };

  const handleDeleteRoom = async () => {
    if (!window.confirm("Are you sure you want to delete this room? All players will be kicked.")) {
      return;
    }

    try {
      const result = await socketService.deleteRoom(roomId || 1);
      
      if (result.success) {
        alert("Room deleted successfully");
        navigate("/lobby");
      } else {
        alert(`Failed to delete room: ${result.error}`);
      }
    } catch (error) {
      alert("Error deleting room");
    }
  };

  const handleLeaveRoom = () => {
    if (window.confirm("Leave this room?")) {
      navigate("/lobby");
    }
  };

  const handleMemberKicked = (memberId) => {
    // Refresh member list after kick
    setMembers(prev => prev.filter(m => m.id !== memberId));
  };

  return (
    <div 
      className="waiting-room" 
      style={{ 
        backgroundImage: "url('/bg/waitingroom.png')",
        backgroundSize: '100% 100%',
        backgroundPosition: 'center',
        backgroundRepeat: 'no-repeat',
        backgroundAttachment: 'fixed'
      }}
    >
      {/* Countdown Overlay */}
      {countdown !== null && countdown > 0 && (
        <div className="countdown-overlay">
          <div className="countdown-number">{countdown}</div>
          <div className="countdown-text">Game Starting...</div>
        </div>
      )}

      {/* Phần Header phía trên cùng */}
      <UserCard /> 
      <RoomTitle title="Waiting Room" /> 
      
      {/* Nội dung chính chia làm 3 cột */}
      <div className="waiting-room-content"> 
        
        {/* CỘT 1: GAME RULES (BÊN TRÁI) */}
        <div className="wr-left">
          <GameRulesPanel 
            isHost={roomInfo.isHost} 
            roomId={roomId}
            initialRules={rules}
          />
        </div>

        {/* CỘT 2: MEMBER LIST (Ở GIỮA) */}
        <div className="wr-center">
          <MemberListPanel 
            isHost={roomInfo.isHost}
            roomId={roomId}
            roomName={roomInfo.name} 
            roomCode={roomInfo.code}
            members={members}
            maxPlayers={rules.maxPlayers}
            onMemberKicked={handleMemberKicked}
          />
        </div>

        {/* CỘT 3: CÁC NÚT HÀNH ĐỘNG (BÊN PHẢI) */}
        <div className="wr-right-actions">
          {roomInfo.isHost && (
            <>
              <button 
                className="start-game-btn" 
                onClick={handleStartGame}
                disabled={startingGame}
              >
                {startingGame ? 'STARTING...' : 'START GAME'}
              </button>
              <button 
                className="delete-room-btn" 
                onClick={handleDeleteRoom}
              >
                DELETE ROOM
              </button>
            </>
          )}
          <button className="invite-btn">INVITE FRIENDS</button>
          <button className="leave-btn" onClick={handleLeaveRoom}>
            LEAVE ROOM
          </button>
        </div>

      </div>
    </div>
  );
}