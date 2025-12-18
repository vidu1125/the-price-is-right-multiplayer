# Frontend Integration with Host Features

This document explains the complete architecture following APPLICATION DESIGN requirements.

## 🏗️ Architecture Flow (UPDATED - TUÂN THỦ APPLICATION DESIGN)

### **LUỒNG CHÍNH - Client-Server RAW SOCKET:**

```
C Terminal Client → RAW TCP SOCKET → C Server → IPC → Python Backend → Database
                    (Binary Protocol)     (Port 8888)
```

### **LUỒNG PHỤ - Web UI:**

```
React Frontend → HTTP/JSON → Python Backend → RAW TCP SOCKET → C Server
                                               (Binary Protocol)
```

**Giải thích:**
- ✅ **C Client ↔ C Server:** Sử dụng RAW TCP SOCKET với Binary Protocol (đáp ứng yêu cầu môn học)
- ✅ **React UI ↔ Backend:** Sử dụng HTTP/JSON (browser không hỗ trợ raw socket)
- ✅ **Backend ↔ C Server:** Python Backend gửi Binary Protocol qua RAW TCP SOCKET
- ✅ **C Server ↔ Backend Services:** IPC (Unix Socket) để xử lý business logic

## 📋 Implementation Summary

### ✅ Created Files

**Backend:**
1. **[Backend/app/services/socket_client.py](Backend/app/services/socket_client.py)** - RAW TCP Socket client (Python → C Server)
2. **[Backend/app/routes/network_routes.py](Backend/app/routes/network_routes.py)** - HTTP API → Socket proxy

**Frontend:**
3. **[Frontend/src/services/socketService.js](Frontend/src/services/socketService.js)** - HTTP client wrapper

**C Client (Terminal):**
4. **[Network/client/src/client_main.c](Network/client/src/client_main.c)** - C Terminal application với RAW TCP Socket
5. **[Network/client/Makefile](Network/client/Makefile)** - Build configuration

### ✅ Updated Components

**React UI:**
1. **[Frontend/src/components/Lobby/CreateRoomPanel.js](Frontend/src/components/Lobby/CreateRoomPanel.js)** - Create Room UI
2. **[Frontend/src/components/Room/WaitingRoom/GameRulesPanel.js](Frontend/src/components/Room/WaitingRoom/GameRulesPanel.js)** - Set Rules UI
3. **[Frontend/src/components/Room/WaitingRoom/MemberListPanel.js](Frontend/src/components/Room/WaitingRoom/MemberListPanel.js)** - Kick Member UI
4. **[Frontend/src/components/Room/WaitingRoom/WaitingRoom.js](Frontend/src/components/Room/WaitingRoom/WaitingRoom.js)** - Start Game & Delete Room UI

---

## 🎮 Use Case Integrations

### 1️⃣ Create Room

**Frontend Component:** [CreateRoomPanel.js](Frontend/src/components/Lobby/CreateRoomPanel.js#L14-L52)

**Flow:**
```javascript
User clicks "Create Room" 
→ Form modal opens
→ User fills: name, visibility, mode, max_players, round_time, wager
→ socketService.createRoom(formData)
→ POST /api/network/command { command: "CREATE_ROOM", data: {...} }
→ Python: create_room() service
→ Returns: { room_id, room_code, host_id }
→ Frontend: Shows room code, navigates to WaitingRoom
```

**Code:**
```javascript
const result = await socketService.createRoom({
  name: "Fun Room",
  visibility: "public",
  mode: "scoring",
  maxPlayers: 4,
  roundTime: 30,
  wagerEnabled: false
});

// Response: { success: true, room: { id, code, hostId } }
```

---

### 2️⃣ Set Rules

**Frontend Component:** [GameRulesPanel.js](Frontend/src/components/Room/WaitingRoom/GameRulesPanel.js#L22-L47)

**Flow:**
```javascript
Host clicks "edit" button
→ Edit mode enabled
→ User changes: mode, max_players, round_time, wager
→ Clicks "Save"
→ socketService.setRules(roomId, rules)
→ POST /api/network/command { command: "SET_RULE", data: {...} }
→ Python: set_rule() service
→ Returns: { success: true }
→ Frontend: Shows "Rules updated", exits edit mode
```

**Code:**
```javascript
const result = await socketService.setRules(roomId, {
  mode: "elimination",
  maxPlayers: 6,
  roundTime: 60,
  wagerEnabled: true
});
```

**Validation:**
- Elimination mode requires `max_players >= 4`
- Round time: 15-60 seconds
- Only host can edit rules

---

### 3️⃣ Kick Member

**Frontend Component:** [MemberListPanel.js](Frontend/src/components/Room/WaitingRoom/MemberListPanel.js#L21-L44)

**Flow:**
```javascript
Host sees "×" button on non-host player cards
→ Clicks kick button
→ Confirmation dialog appears
→ User confirms
→ socketService.kickMember(roomId, memberId)
→ POST /api/network/command { command: "KICK_MEMBER", data: {...} }
→ Python: kick_member() service
→ Returns: { success: true }
→ Frontend: Removes member from list, shows alert
```

**Code:**
```javascript
const handleKick = async (memberId, memberName) => {
  if (!confirm(`Kick ${memberName}?`)) return;
  
  const result = await socketService.kickMember(roomId, memberId);
  // Member removed from UI
};
```

**UI Features:**
- Only visible to host
- Button disabled during kick operation
- Confirmation dialog prevents accidental kicks
- Cannot kick host (button not shown)

---

### 4️⃣ Start Game

**Frontend Component:** [WaitingRoom.js](Frontend/src/components/Room/WaitingRoom/WaitingRoom.js#L38-L68)

**Flow:**
```javascript
Host clicks "START GAME" button
→ Confirmation dialog
→ socketService.startGame(roomId)
→ POST /api/network/command { command: "START_GAME", data: {...} }
→ Python: start_game() service
→ Returns: { match_id, countdown_ms, server_timestamp_ms, game_start_timestamp_ms }
→ Frontend: Shows countdown overlay (3... 2... 1...)
→ Navigates to /game/:matchId
```

**Code:**
```javascript
const result = await socketService.startGame(roomId);

// Response:
// {
//   success: true,
//   matchId: 123,
//   countdown: 3000,
//   serverTime: 1702886400000,
//   gameStartTime: 1702886403000
// }

// Show countdown
setCountdown(3);
const interval = setInterval(() => {
  setCountdown(prev => prev - 1);
  if (prev <= 1) navigate(`/game/${matchId}`);
}, 1000);
```

**Features:**
- Only host can start
- Validates all players are ready (backend)
- Client-side countdown with server timestamps
- Non-blocking (server continues handling other requests)
- Countdown overlay covers entire screen

---

### 5️⃣ Delete Room

**Frontend Component:** [WaitingRoom.js](Frontend/src/components/Room/WaitingRoom/WaitingRoom.js#L70-L84)

**Flow:**
```javascript
Host clicks "DELETE ROOM" button
→ Confirmation dialog: "All players will be kicked"
→ socketService.deleteRoom(roomId)
→ POST /api/network/command { command: "DELETE_ROOM", data: {...} }
→ Python: delete_room() service
→ Returns: { success: true }
→ Frontend: Shows alert, navigates to /lobby
```

**Code:**
```javascript
const handleDeleteRoom = async () => {
  if (!confirm("Delete room? All players will be kicked.")) return;
  
  const result = await socketService.deleteRoom(roomId);
  navigate("/lobby");
};
```

**Cascade Effects:**
- Removes all `room_members` entries
- Deletes `Match` (if created)
- Kicks all players (broadcast notification)
- Only host has permission

---

## 🔌 Backend Proxy Endpoints

### POST `/api/network/connect`
Establish session (placeholder for WebSocket upgrade)

**Request:**
```json
{
  "account_id": 123
}
```

**Response:**
```json
{
  "success": true,
  "session_id": "session_123"
}
```

---

### POST `/api/network/command`
Universal command router

**Request:**
```json
{
  "command": "CREATE_ROOM" | "SET_RULE" | "KICK_MEMBER" | "START_GAME" | "DELETE_ROOM",
  "account_id": 123,
  "data": { ... }
}
```

**Response:**
```json
{
  "success": true,
  // ... command-specific response
}
```

---

## 🎨 UI/UX Features

### Create Room Modal
- Form validation (name required, max_players 4-6)
- Loading state during creation
- Shows room code on success
- Closes modal after creation

### Game Rules Panel
- Edit button (host only)
- Visual mode toggle (Eliminate/Scoring)
- Wager ON/OFF badges
- Round time presets (Slow/Normal/Fast)
- Save/Cancel buttons in edit mode

### Member List
- Crown icon for host
- Kick button (× symbol) on member cards
- Ready/Waiting status tags
- Empty slot placeholders
- Player count footer

### Start Game
- Confirmation dialog
- Full-screen countdown overlay
- Countdown animation (3... 2... 1...)
- Auto-navigation to game screen
- Button disabled during countdown

### Delete Room
- Red warning confirmation
- Explains "all players kicked"
- Returns to lobby after deletion

---

## 🧪 Testing Guide

### 1. Test Create Room
```bash
# Start backend
cd Backend
python app/main.py

# Start frontend
cd Frontend
npm start

# Test:
# 1. Go to http://localhost:3000/lobby
# 2. Click "Create new room"
# 3. Fill form and submit
# 4. Should show room code
```

### 2. Test Set Rules
```bash
# Prerequisites: Create a room first

# Test:
# 1. Go to WaitingRoom
# 2. Click "edit" on Game Rules panel
# 3. Change mode to Elimination
# 4. Set max players to 6
# 5. Click "Save"
# 6. Should show success alert
```

### 3. Test Kick Member
```bash
# Prerequisites: Have 2+ players in room

# Test:
# 1. As host in WaitingRoom
# 2. See "×" button on other player cards
# 3. Click "×"
# 4. Confirm in dialog
# 5. Member disappears from list
```

### 4. Test Start Game
```bash
# Prerequisites: Have 4+ players ready

# Test:
# 1. As host click "START GAME"
# 2. Confirm in dialog
# 3. See countdown overlay (3... 2... 1...)
# 4. Auto-navigate to /game/:matchId
```

### 5. Test Delete Room
```bash
# Test:
# 1. As host click "DELETE ROOM"
# 2. Confirm warning dialog
# 3. Navigate back to /lobby
# 4. Room should not appear in room list
```

---

## 🔧 Configuration

### Environment Variables

**Frontend (.env):**
```bash
REACT_APP_NETWORK_API=http://localhost:5000/api/network
REACT_APP_API_URL=http://localhost:5000/api
```

**Backend (.env):**
```bash
DATABASE_URL=postgresql://postgres.ecnfnieobrpfbosucbdz:123456@aws-1-ap-southeast-1.pooler.supabase.com:5432/postgres
```

---

## 📝 Notes

### Why HTTP Proxy Instead of Direct TCP?

**Problem:** Browsers cannot establish raw TCP socket connections (security restriction)

**Solutions:**
1. ✅ **HTTP Proxy** (implemented): Frontend → HTTP → Backend → IPC → C Server
2. WebSocket Bridge: Frontend → WS → Proxy → TCP → C Server
3. Direct Backend: Skip C server entirely for host features

**Current Implementation:**
- Host features use HTTP → IPC → Python (no C server needed)
- C server is optional for future game logic
- Clean separation: Frontend ↔ Backend API ↔ Services

### Future Improvements

- [ ] Add WebSocket for real-time notifications (broadcast)
- [ ] Implement session management (JWT tokens)
- [ ] Add loading states to all buttons
- [ ] Error handling with toast notifications
- [ ] Add room refresh polling
- [ ] Implement invite friends feature
- [ ] Add sound effects for countdown
- [ ] Persist room state in localStorage

---

## ✅ Status

All 5 host features are fully integrated:
- ✅ Create Room - [CreateRoomPanel.js](Frontend/src/components/Lobby/CreateRoomPanel.js)
- ✅ Set Rules - [GameRulesPanel.js](Frontend/src/components/Room/WaitingRoom/GameRulesPanel.js)
- ✅ Kick Member - [MemberListPanel.js](Frontend/src/components/Room/WaitingRoom/MemberListPanel.js)
- ✅ Start Game - [WaitingRoom.js](Frontend/src/components/Room/WaitingRoom/WaitingRoom.js)
- ✅ Delete Room - [WaitingRoom.js](Frontend/src/components/Room/WaitingRoom/WaitingRoom.js)

**Ready for testing! 🚀**
