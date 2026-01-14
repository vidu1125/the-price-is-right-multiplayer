// WaitingRoom.js
import "./WaitingRoom.css";
import UserCard from "../../Lobby/UserCard";
import RoomTitle from "./RoomTitle";
import GameRulesPanel from "./GameRulesPanel";
import MemberListPanel from "./MemberListPanel";
import { useLocation, useNavigate } from "react-router-dom";
import { useState, useEffect } from "react";
import { registerHandler } from "../../../network/receiver";
import { OPCODE } from "../../../network/opcode";
import { startGame } from "../../../services/gameService";

export default function WaitingRoom() {
  const location = useLocation();
  const navigate = useNavigate();

  // 1. Initialize State from Navigation Data
  const { roomId, roomCode, roomName, isHost, hostId, gameRules: passedRules } = location.state || {};

  console.log("[WaitingRoom] location.state:", location.state);
  console.log("[WaitingRoom] roomName from state:", roomName);
  console.log("[WaitingRoom] gameRules from state:", passedRules);

  // Get current user profile to add as host
  const profile = JSON.parse(localStorage.getItem('profile') || '{}');
  console.log("[WaitingRoom] profile from localStorage:", profile);

  const [room, setRoom] = useState({
    id: roomId,
    code: roomCode,
    name: roomName || "My Room",
    hostId: hostId || profile.account_id || null,
    players: (location.state && location.state.players) ? location.state.players : (isHost ? [{
      account_id: profile.account_id,
      name: profile.name || 'Host',
      is_host: true,
      is_ready: false
    }] : []),
    rules: passedRules || {
      maxPlayers: 5,
      mode: "scoring",
      wagerMode: false,
      roundTime: 15
    }
  });

  console.log("[WaitingRoom] Initial room state:", room);

  // Invite Modal State
  const [isInviteModalOpen, setIsInviteModalOpen] = useState(false);
  const [invitePlayerId, setInvitePlayerId] = useState("");

  useEffect(() => {
    // 2. Validate Room ID
    if (!roomId) {
      console.error("[WaitingRoom] No room ID provided, redirecting to lobby");
      navigate('/lobby');
      return;
    }

    console.log(`[WaitingRoom] Listening for updates for Room ${roomId} (${roomCode})`);

    // --- SOCKET NOTIFICATION HANDLERS ---

    // 1. NTF_PLAYER_LIST
    registerHandler(OPCODE.NTF_PLAYER_LIST, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const data = JSON.parse(text);
        if (data.members && Array.isArray(data.members)) {
          setRoom(prev => ({ ...prev, players: data.members }));
        }
      } catch (e) { console.error("Parse error:", e); }
    });

    // 2. NTF_PLAYER_JOINED
    registerHandler(OPCODE.NTF_PLAYER_JOINED, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const newPlayer = JSON.parse(text);
        setRoom(prev => {
          if (prev.players.find(m => m.account_id === newPlayer.account_id)) return prev;
          return { ...prev, players: [...prev.players, newPlayer] };
        });
      } catch (e) { }
    });

    // 3. NTF_PLAYER_LEFT
    registerHandler(OPCODE.NTF_PLAYER_LEFT, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const { account_id } = JSON.parse(text);
        setRoom(prev => ({
          ...prev,
          players: prev.players.filter(m => m.account_id !== account_id)
        }));
      } catch (e) { }
    });

    // 4. NTF_RULES_CHANGED
    registerHandler(OPCODE.NTF_RULES_CHANGED, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const newRules = JSON.parse(text);
        setRoom(prev => ({
          ...prev,
          rules: { ...prev.rules, ...newRules }
        }));
      } catch (e) { }
    });

    // 5. NTF_PLAYER_READY
    registerHandler(OPCODE.NTF_PLAYER_READY, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const data = JSON.parse(text);
        setRoom(prev => ({
          ...prev,
          players: prev.players.map(p =>
            p.account_id === data.account_id ? { ...p, is_ready: data.is_ready } : p
          )
        }));
      } catch (e) { }
    });

    // 6. NTF_ROOM_CLOSED
    registerHandler(OPCODE.NTF_ROOM_CLOSED, () => {
      alert("Host closed the room.");
      navigate('/lobby');
    });

    // 7. NTF_GAME_START
    registerHandler(OPCODE.NTF_GAME_START || 0x02C4, (payload) => {
      try {
        const text = new TextDecoder().decode(payload);
        const data = JSON.parse(text);
        const matchId = data.match_id;
        const profile = JSON.parse(localStorage.getItem('profile') || '{}');
        const playerId = profile.account_id;
        const playerName = profile.name || `Player${playerId}`;

        navigate(`/round?match_id=${matchId}&player_id=${playerId}&name=${encodeURIComponent(playerName)}`, {
          state: { roomId, roomCode, isHost, matchId, playerId, playerName }
        });
      } catch (e) {
        navigate('/round', { state: { roomId, roomCode, isHost } });
      }
    });

    // Handle generic success/error for приглашение (invite)
    registerHandler(OPCODE.RES_SUCCESS || 0x00C8, (payload) => {
      const text = new TextDecoder().decode(payload);
      if (text.includes("Invitation sent")) {
        alert("Invitation successfully sent!");
        setIsInviteModalOpen(false);
        setInvitePlayerId("");
      }
    });

    registerHandler(OPCODE.ERR_BAD_REQUEST || 0x0190, (payload) => {
      const text = new TextDecoder().decode(payload);
      // We check if it's related to invitation
      if (text.includes("user is currently") || text.includes("user is currently busy")) {
        alert("Error: " + text);
      }
    });

  }, [roomId, roomCode, navigate]);

  const handleRulesChange = (newRules) => {
    setRoom(prev => ({ ...prev, rules: newRules }));
  };

  const handleStartGame = () => {
    startGame(room.id);
  };

  const handleInvitePlayer = () => {
    const targetId = parseInt(invitePlayerId);
    if (isNaN(targetId)) {
      alert("Please enter a valid Player ID (number).");
      return;
    }
    const { invitePlayer } = require('../../../services/roomService');
    invitePlayer(targetId, room.id);
  };

  if (!roomId) return null;

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
      <UserCard />
      <RoomTitle title={`Room Code: ${room.code}`} />

      <div className="waiting-room-content">
        <div className="wr-left">
          <GameRulesPanel
            isHost={isHost}
            roomId={room.id}
            gameRules={room.rules}
            currentPlayerCount={room.players.length}
            onRulesChange={handleRulesChange}
          />
        </div>

        <div className="wr-center">
          <MemberListPanel
            isHost={isHost}
            roomId={room.id}
            hostId={room.hostId}
            roomName={room.name || "My Room"}
            roomCode={room.code}
            maxPlayers={room.rules.maxPlayers}
            members={room.players}
            onRefresh={() => { }}
          />
        </div>

        <div className="wr-right-actions">
          {isHost && (
            <button className="start-game-btn" onClick={handleStartGame}>START GAME</button>
          )}
          <button className="invite-btn" onClick={() => setIsInviteModalOpen(true)}>INVITE FRIENDS</button>

          <button
            className={`ready-toggle-btn ${room.players.find(p => p.account_id === profile.account_id)?.is_ready ? 'ready' : 'not-ready'}`}
            onClick={() => {
              const { toggleReady } = require('../../../services/roomService');
              toggleReady();
            }}
          >
            {room.players.find(p => p.account_id === profile.account_id)?.is_ready ? 'NOT READY' : 'READY'}
          </button>

          <button className="leave-btn" onClick={() => navigate('/lobby')}>LEAVE ROOM</button>
        </div>
      </div>

      {/* Invite Modal */}
      {isInviteModalOpen && (
        <div className="invite-modal-overlay">
          <div className="invite-modal">
            <h3>INVITE PLAYER</h3>
            <div className="invite-input-group">
              <label>Enter Player ID:</label>
              <input
                type="number"
                value={invitePlayerId}
                onChange={(e) => setInvitePlayerId(e.target.value)}
                placeholder="Ex: 123"
              />
            </div>
            <div className="invite-modal-actions">
              <button className="confirm-btn" onClick={handleInvitePlayer}>INVITE</button>
              <button className="cancel-btn" onClick={() => setIsInviteModalOpen(false)}>CANCEL</button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}