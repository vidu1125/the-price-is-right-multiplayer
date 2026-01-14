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

  useEffect(() => {
    // 2. Validate Room ID
    if (!roomId) {
      console.error("[WaitingRoom] No room ID provided, redirecting to lobby");
      navigate('/lobby');
      return;
    }

    console.log(`[WaitingRoom] Listening for updates for Room ${roomId} (${roomCode})`);
    console.log("[WaitingRoom] Registering NTF_PLAYER_LIST handler");

    // --- SOCKET NOTIFICATION HANDLERS ---

    // 1. NTF_PLAYER_LIST (Snapshot of members)
    registerHandler(OPCODE.NTF_PLAYER_LIST, (payload) => {
      console.log("[WaitingRoom] NTF_PLAYER_LIST handler called!");
      const text = new TextDecoder().decode(payload);
      console.log("[WaitingRoom] Raw payload:", text);
      try {
        const data = JSON.parse(text);
        console.log("[WaitingRoom] Parsed Player List:", data);

        if (data.members && Array.isArray(data.members)) {
          console.log("[WaitingRoom] Updating players:", data.members);
          setRoom(prev => ({
            ...prev,
            players: data.members
          }));
        }
      } catch (e) {
        console.error("[WaitingRoom] Failed to parse player list:", e);
      }
    });

    // 2. NTF_PLAYER_JOINED
    registerHandler(OPCODE.NTF_PLAYER_JOINED, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const newPlayer = JSON.parse(text);
        console.log("[NTF] Player Joined:", newPlayer);
        setRoom(prev => {
          // Avoid duplicates
          if (prev.players.find(m => m.id === newPlayer.id)) return prev;
          return {
            ...prev,
            players: [...prev.players, newPlayer]
          };
        });
      } catch (e) {
        console.error("Failed to parse join ntf:", e);
      }
    });

    // 3. NTF_PLAYER_LEFT
    registerHandler(OPCODE.NTF_PLAYER_LEFT, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const { account_id } = JSON.parse(text);
        console.log("[NTF] Player Left:", account_id);
        setRoom(prev => ({
          ...prev,
          players: prev.players.filter(m => m.account_id !== account_id)
        }));
      } catch (e) {
        console.error("Failed to parse leave ntf:", e);
      }
    });

    // 4. NTF_RULES_CHANGED
    registerHandler(OPCODE.NTF_RULES_CHANGED, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const newRules = JSON.parse(text);
        console.log("[NTF] Rules Changed:", newRules);
        setRoom(prev => ({
          ...prev,
          rules: { ...prev.rules, ...newRules }
        }));
      } catch (e) {
        console.error("Failed to parse rules ntf:", e);
      }
    });

    // 5. NTF_PLAYER_READY
    registerHandler(OPCODE.NTF_PLAYER_READY, (payload) => {
      const text = new TextDecoder().decode(payload);
      try {
        const data = JSON.parse(text);
        console.log("[NTF] Player Ready:", data);

        // Update specific player's ready state
        setRoom(prev => ({
          ...prev,
          players: prev.players.map(p =>
            p.account_id === data.account_id
              ? { ...p, is_ready: data.is_ready }
              : p
          )
        }));

        console.log(`🔔 Player ${data.account_id} is now ${data.is_ready ? 'READY' : 'NOT READY'}`);
      } catch (e) {
        console.error("Failed to parse player ready ntf:", e);
      }
    });

    // 6. NTF_ROOM_CLOSED
    registerHandler(OPCODE.NTF_ROOM_CLOSED, (payload) => {
      alert("Host closed the room.");
      navigate('/lobby');
    });

    // 6. NTF_GAME_START
    registerHandler(OPCODE.NTF_GAME_START || 0x02C4, (payload) => {
      console.log("[NTF] Game Started! Parsing payload...");
      try {
        const text = new TextDecoder().decode(payload);
        const data = JSON.parse(text);
        const matchId = data.match_id;
        console.log("[NTF] Match ID:", matchId);

        // Get player_id from profile in localStorage
        const profile = JSON.parse(localStorage.getItem('profile') || '{}');
        const playerId = profile.account_id;
        const playerName = profile.name || `Player${playerId}`;

        console.log("[NTF] Player ID:", playerId, "Name:", playerName);
        navigate(`/round?match_id=${matchId}&player_id=${playerId}&name=${encodeURIComponent(playerName)}`, {
          state: { roomId, roomCode, isHost, matchId, playerId, playerName }
        });
      } catch (e) {
        console.error("[NTF] Failed to parse NTF_GAME_START:", e);
        // Fallback without match_id
        navigate('/round', { state: { roomId, roomCode, isHost } });
      }
    });

    // Cleanup listeners? 
    // Ideally receiver.js should support unregister, but if not, 
    // subsequent renders normally override if using named keys or singleton.
    // Assuming registerHandler replaces old handler for same opcode.

  }, [roomId, roomCode, navigate]);

  // Handle Game Rules UI Change (only for Host)
  const handleRulesChange = (newRules) => {
    // NOTE: Here we should SEND packet to server to update rules
    // For now, we optimistically update local state or wait for server Ack?
    // Better to just send packet. 
    // Since sendPacket is not fully integrated in this snippet, we just update local state
    // but ideally this should trigger a CMD_SET_RULE
    console.log("Host changed rules:", newRules);

    // Optimistic update
    setRoom(prev => ({ ...prev, rules: newRules }));

    // TODO: Implement sendPacket(OPCODE.CMD_SET_RULE, rulesPayload);
  };


  // Handle Start Game
  const handleStartGame = () => {
    console.log("Host initiating game start...");
    startGame(room.id);
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

        {/* LEFT: Game Rules */}
        <div className="wr-left">
          <GameRulesPanel
            isHost={isHost}
            roomId={room.id}
            gameRules={room.rules}
            onRulesChange={handleRulesChange}
          />
        </div>

        {/* CENTER: Members */}
        <div className="wr-center">
          <MemberListPanel
            isHost={isHost}
            roomId={room.id}
            hostId={room.hostId} // Might be null initially
            roomName={room.name || "My Room"}
            roomCode={room.code}
            maxPlayers={room.rules.maxPlayers}
            members={room.players}
            onRefresh={() => { }} // Disabled manual refresh
          />
        </div>

        {/* RIGHT: Actions */}
        <div className="wr-right-actions">
          {isHost && (
            <button className="start-game-btn" onClick={handleStartGame}>START GAME</button>
          )}
          <button className="invite-btn">INVITE FRIENDS</button>

          {/* READY BUTTON */}
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
    </div>
  );
}