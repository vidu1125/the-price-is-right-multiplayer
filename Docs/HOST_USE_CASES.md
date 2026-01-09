# HOST USE CASES - Technical Design Document

> **Branch:** `feature/hostv3-clean`  
> **Status:** ✅ Tested & Working  
> **Date:** January 9, 2026

---

## 🎯 OVERVIEW

Host có 4 use cases chính được implement với **binary protocol** và **server-driven architecture**:

1. **CREATE ROOM** - Tạo phòng mới
2. **SET RULES** - Thay đổi game settings
3. **KICK MEMBER** - Đuổi người chơi
4. **START GAME** - Bắt đầu trận đấu

---

## 🏗️ ARCHITECTURE

### **Component Stack**

```
┌─────────────────────────────────────────────────────────┐
│  FRONTEND (React)                                       │
│  - hostService.js: Binary payload encoding              │
│  - WaitingRoom.jsx: UI components                      │
└─────────────────────────────────────────────────────────┘
                        ↓ WebSocket
┌─────────────────────────────────────────────────────────┐
│  WS-BRIDGE (Node.js)                                    │
│  - Protocol translation: WS ↔ TCP                       │
└─────────────────────────────────────────────────────────┘
                        ↓ TCP Socket
┌─────────────────────────────────────────────────────────┐
│  NETWORK SERVER (C)                                     │
│  ┌───────────────────────────────────────────────────┐  │
│  │ room_handler.c                                    │  │
│  │ - handle_create_room()                            │  │
│  │ - handle_set_rules()                              │  │
│  │ - handle_kick_member()                            │  │
│  │ - handle_start_game()                             │  │
│  └───────────────────────────────────────────────────┘  │
│           ↓ Business Logic                              │
│  ┌───────────────────────────────────────────────────┐  │
│  │ room_repo.c (DB Operations)                       │  │
│  └───────────────────────────────────────────────────┘  │
│           ↓ State Management                            │
│  ┌───────────────────────────────────────────────────┐  │
│  │ room_manager.c (Memory Layer)                     │  │
│  │ - Track members + is_ready state                  │  │
│  │ - room_broadcast() for notifications              │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                        ↓ REST API
┌─────────────────────────────────────────────────────────┐
│  SUPABASE (PostgreSQL)                                  │
│  - rooms table                                          │
│  - room_members table                                   │
│  - RPC functions                                        │
└─────────────────────────────────────────────────────────┘
```

---

## 📊 DATA STRUCTURES

### **Memory Layer (room_manager.c)**

```c
typedef struct {
    int client_fd;          // Socket descriptor (volatile)
    uint32_t account_id;    // User ID from DB (persistent)
    bool is_ready;          // Ready state (volatile)
} RoomMember;

typedef struct {
    int room_id;
    RoomMember members[MAX_ROOM_MEMBERS];  // Max 6
    int member_count;
} RoomState;
```

**Key Points:**
- ✅ `is_ready` state **CHỈ LƯU TRONG MEMORY**
- ✅ Host luôn `is_ready = true` khi create room
- ✅ `client_fd` mapping cho real-time broadcast

---

### **Database Layer (PostgreSQL)**

```sql
-- rooms table
id              SERIAL PRIMARY KEY
name            VARCHAR(100)
code            VARCHAR(10) UNIQUE        -- 6-char join code
visibility      VARCHAR(20)               -- 'public' | 'private'
host_id         INTEGER REFERENCES accounts(id)
status          VARCHAR(20)               -- 'waiting' | 'playing' | 'closed'
mode            VARCHAR(20)               -- 'scoring' | 'elimination'
max_players     INTEGER DEFAULT 5
wager_mode      BOOLEAN DEFAULT TRUE
created_at      TIMESTAMP
updated_at      TIMESTAMP

-- room_members table
room_id         INTEGER REFERENCES rooms(id)
account_id      INTEGER REFERENCES accounts(id)
joined_at       TIMESTAMP
PRIMARY KEY (room_id, account_id)
```

**⚠️ Note:** `is_ready` **KHÔNG LƯU** trong DB

---

## 🔌 PROTOCOL DEFINITIONS

### **Message Header (16 bytes)**

```c
typedef struct PACKED {
    uint16_t magic;       // 0x4347 ("CG")
    uint8_t  version;     // 0x01
    uint8_t  flags;       // Reserved
    uint16_t command;     // Opcode
    uint16_t reserved;    
    uint32_t seq_num;     // Sequence number
    uint32_t length;      // Payload size
} MessageHeader;
```

### **Payload Structures**

```c
// CREATE ROOM (72 bytes)
typedef struct PACKED {
    char     name[64];          // UTF-8 null-terminated
    uint8_t  visibility;        // 0=public, 1=private
    uint8_t  mode;              // 0=scoring, 1=elimination
    uint8_t  max_players;       // 2-8
    uint8_t  wager_enabled;     // 0=false, 1=true
    uint8_t  reserved[4];
} CreateRoomPayload;

// SET RULES (8 bytes)
typedef struct PACKED {
    uint32_t room_id;           // Network byte order
    uint8_t  mode;
    uint8_t  max_players;
    uint8_t  wager_enabled;
    uint8_t  visibility;
} SetRulesPayload;

// KICK MEMBER (8 bytes)
typedef struct PACKED {
    uint32_t room_id;           // Network byte order
    uint32_t target_id;         // Account ID to kick
} KickMemberPayload;

// ROOM ID (4 bytes) - for CLOSE/START
typedef struct PACKED {
    uint32_t room_id;           // Network byte order
} RoomIDPayload;
```

---

## 🎮 USE CASE 1: CREATE ROOM

### **Frontend Code**

```javascript
// hostService.js
export function createRoom(roomData) {
    const buffer = new ArrayBuffer(72);
    const view = new DataView(buffer);
    
    // Encode name (64 bytes)
    const nameBytes = encodeString(roomData.name || "New Room", 64);
    new Uint8Array(buffer, 0, 64).set(nameBytes);
    
    // Pack fields
    view.setUint8(64, roomData.visibility === 'private' ? 1 : 0);
    view.setUint8(65, roomData.mode === 'elimination' ? 1 : 0);
    view.setUint8(66, roomData.maxPlayers || 6);
    view.setUint8(67, roomData.wagerEnabled ? 1 : 0);
    
    sendPacket(OPCODE.CMD_CREATE_ROOM, buffer);
}
```

### **Server Handler**

```c
// room_handler.c
void handle_create_room(int client_fd, MessageHeader *req, const char *payload) {
    // 1. Validate payload
    if (req->length != sizeof(CreateRoomPayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload");
        return;
    }
    
    // 2. Extract & validate
    CreateRoomPayload data;
    memcpy(&data, payload, sizeof(data));
    
    // 3. Get account_id from session
    int32_t account_id = get_account_id(client_fd);
    if (account_id < 0) {
        send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not authenticated");
        return;
    }
    
    // 4. Create in DB
    uint32_t room_id = 0;
    char result[2048];
    int rc = room_repo_create(
        room_name,
        data.visibility,
        data.mode,
        data.max_players,
        data.wager_enabled,
        account_id,          // ✅ From session
        result,
        sizeof(result),
        &room_id
    );
    
    if (rc != 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to create");
        return;
    }
    
    // 5. Track in memory
    room_add_member(room_id, client_fd, account_id, true);
    //                                               ^^^^ is_host=true
    
    // 6. Response to host
    forward_response(client_fd, req, RES_ROOM_CREATED, result, strlen(result));
    
    // 7. Broadcast initial state
    room_broadcast(room_id, NTF_RULES_CHANGED, rules_json, strlen(rules_json), -1);
    room_broadcast(room_id, NTF_PLAYER_LIST, players_json, strlen(players_json), -1);
}
```

### **Database Operation**

```c
// room_repo.c
int room_repo_create(..., int32_t host_account_id, ...) {
    // Generate 6-char code
    char code[7];
    generate_room_code(code);
    
    // POST /rooms
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "name", name);
    cJSON_AddStringToObject(payload, "code", code);
    cJSON_AddStringToObject(payload, "visibility", visibility ? "private" : "public");
    cJSON_AddNumberToObject(payload, "host_id", host_account_id);  // ✅ Real ID
    cJSON_AddStringToObject(payload, "status", "waiting");
    cJSON_AddStringToObject(payload, "mode", mode ? "elimination" : "scoring");
    cJSON_AddNumberToObject(payload, "max_players", max_players);
    cJSON_AddBoolToObject(payload, "wager_mode", wager_enabled);
    
    db_post("rooms", payload, &response);
    
    // Extract room_id from response
    *room_id = cJSON_GetObjectItem(first, "id")->valueint;
    
    // POST /room_members (add host)
    cJSON *member = cJSON_CreateObject();
    cJSON_AddNumberToObject(member, "room_id", *room_id);
    cJSON_AddNumberToObject(member, "account_id", host_account_id);  // ✅ Real ID
    db_post("room_members", member, &member_resp);
    
    return 0;
}
```

### **Memory State**

```c
// room_manager.c
void room_add_member(int room_id, int client_fd, uint32_t account_id, bool is_host) {
    RoomMember *member = &room->members[room->member_count++];
    member->client_fd = client_fd;
    member->account_id = account_id;
    member->is_ready = is_host;  // ✅ Host auto ready
}
```

### **Flow Diagram**

```
User clicks "Create Room"
  ↓
hostService.createRoom()
  → Binary payload (72 bytes)
  → CMD_CREATE_ROOM (0x0200)
  ↓
Server: handle_create_room()
  → Get account_id from session
  → room_repo_create() → DB INSERT (rooms + room_members)
  → room_add_member() → Memory tracking (is_ready=true for host)
  → RES_ROOM_CREATED → Host
  → NTF_RULES_CHANGED → Broadcast
  → NTF_PLAYER_LIST → Broadcast
  ↓
Frontend receives notifications
  → Updates UI with room state
  → Host shows as ready
```

---

## 🎮 USE CASE 2: SET RULES

### **Frontend Code**

```javascript
export function setRules(roomId, rules) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    
    view.setUint32(0, roomId, false);  // Big-endian
    view.setUint8(4, rules.mode === 'elimination' ? 1 : 0);
    view.setUint8(5, rules.maxPlayers || 6);
    view.setUint8(6, rules.wagerMode ? 1 : 0);
    view.setUint8(7, rules.visibility === 'private' ? 1 : 0);
    
    sendPacket(OPCODE.CMD_SET_RULE, buffer);
}
```

### **Server Handler**

```c
void handle_set_rules(int client_fd, MessageHeader *req, const char *payload) {
    SetRulesPayload data;
    memcpy(&data, payload, sizeof(data));
    uint32_t room_id = ntohl(data.room_id);
    
    // Update DB via RPC
    char resp_buf[2048];
    int rc = room_repo_set_rules(
        room_id,
        data.mode,
        data.max_players,
        data.wager_enabled,
        data.visibility,
        resp_buf,
        sizeof(resp_buf)
    );
    
    if (rc != 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to update");
        return;
    }
    
    // Broadcast to ALL members (including host)
    char rules_json[256];
    snprintf(rules_json, sizeof(rules_json),
        "{\"mode\":\"%s\",\"max_players\":%u,\"wager_mode\":%s,\"visibility\":\"%s\"}",
        data.mode ? "elimination" : "scoring",
        data.max_players,
        data.wager_enabled ? "true" : "false",
        data.visibility ? "private" : "public"
    );
    
    room_broadcast(room_id, NTF_RULES_CHANGED, rules_json, strlen(rules_json), -1);
    forward_response(client_fd, req, RES_RULES_UPDATED, resp_buf, strlen(resp_buf));
}
```

### **Database Operation**

```c
int room_repo_set_rules(...) {
    // RPC: update_room_rules
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "p_room_id", room_id);
    cJSON_AddStringToObject(payload, "p_mode", mode ? "elimination" : "scoring");
    cJSON_AddNumberToObject(payload, "p_max_players", max_players);
    cJSON_AddBoolToObject(payload, "p_wager_mode", wager_enabled);
    cJSON_AddStringToObject(payload, "p_visibility", visibility ? "private" : "public");
    
    db_rpc("update_room_rules", payload, &response);
    return 0;
}
```

### **Flow**

```
Host changes rules
  → setRules()
  → CMD_SET_RULE (0x0206)
  → Server updates DB (RPC)
  → RES_RULES_UPDATED → Host
  → NTF_RULES_CHANGED → Broadcast to ALL
  → All clients update UI
```

**⚠️ Note:** Memory state (is_ready) **KHÔNG BỊ RESET** khi rules change

---

## 🎮 USE CASE 3: KICK MEMBER

### **Frontend Code**

```javascript
export function kickMember(roomId, targetId) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    
    view.setUint32(0, roomId, false);
    view.setUint32(4, targetId, false);
    
    sendPacket(OPCODE.CMD_KICK, buffer);
}
```

### **Server Handler**

```c
void handle_kick_member(int client_fd, MessageHeader *req, const char *payload) {
    KickMemberPayload data;
    memcpy(&data, payload, sizeof(data));
    uint32_t room_id = ntohl(data.room_id);
    uint32_t target_id = ntohl(data.target_id);
    
    // Delete from DB
    char resp_buf[1024];
    int rc = room_repo_kick_member(room_id, target_id, resp_buf, sizeof(resp_buf));
    
    if (rc != 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to kick");
        return;
    }
    
    // Notify kicked player
    char kick_notif[64];
    snprintf(kick_notif, sizeof(kick_notif), "{\"account_id\":%u}", target_id);
    room_broadcast(room_id, NTF_MEMBER_KICKED, kick_notif, strlen(kick_notif), -1);
    
    // Pull fresh player list from DB and broadcast
    char state_buf[4096];
    rc = room_repo_get_state(room_id, state_buf, sizeof(state_buf));
    if (rc == 0) {
        cJSON *root = cJSON_Parse(state_buf);
        cJSON *players = cJSON_GetObjectItem(root, "players");
        char *players_str = cJSON_PrintUnformatted(players);
        room_broadcast(room_id, NTF_PLAYER_LIST, players_str, strlen(players_str), -1);
        free(players_str);
        cJSON_Delete(root);
    }
    
    forward_response(client_fd, req, RES_MEMBER_KICKED, resp_buf, strlen(resp_buf));
}
```

### **Database Operation**

```c
int room_repo_kick_member(...) {
    // RPC: kick_member
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "p_room_id", room_id);
    cJSON_AddNumberToObject(payload, "p_account_id", target_id);
    
    db_rpc("kick_member", payload, &response);
    return 0;
}
```

### **Memory Operation**

Frontend catches notification:
```javascript
registerHandler(OPCODE.NTF_MEMBER_KICKED, (payload) => {
    const data = JSON.parse(new TextDecoder().decode(payload));
    const myAccountId = parseInt(localStorage.getItem('account_id'));
    
    if (data.account_id === myAccountId) {
        // I got kicked
        alert('You have been kicked from the room');
        window.location.href = '/lobby';
    }
});
```

### **Flow**

```
Host clicks "Kick" on member
  → kickMember(roomId, targetId)
  → CMD_KICK (0x0204)
  → Server deletes from DB
  → NTF_MEMBER_KICKED → Broadcast (target sees it and leaves)
  → room_repo_get_state() → Get fresh list
  → NTF_PLAYER_LIST → Broadcast updated list
  → RES_MEMBER_KICKED → Host
```

---

## 🎮 USE CASE 4: START GAME

### **Requirements**

✅ All non-host members must be ready  
✅ Minimum 2 players  
✅ Room status = 'waiting'

### **Frontend Code**

```javascript
export function startGame(roomId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    view.setUint32(0, roomId, false);
    
    sendPacket(OPCODE.CMD_START_GAME, buffer);
}
```

### **Server Handler**

```c
void handle_start_game(int client_fd, MessageHeader *req, const char *payload) {
    RoomIDPayload data;
    memcpy(&data, payload, sizeof(data));
    uint32_t room_id = ntohl(data.room_id);
    
    // Check all ready
    int ready_count, total_count;
    if (!room_all_ready(room_id, &ready_count, &total_count)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Not all ready (%d/%d)", ready_count, total_count);
        send_error(client_fd, req, ERR_BAD_REQUEST, msg);
        return;
    }
    
    // Update DB: status = 'playing'
    // Create match record
    // ... (match_handler logic)
    
    // Broadcast game start
    room_broadcast(room_id, NTF_GAME_START, "{}", 2, -1);
    forward_response(client_fd, req, RES_GAME_STARTED, "{\"success\":true}", 17);
}
```

---

## 🔐 AUTHENTICATION & SESSION

### **Session Management**

```c
// Get account_id from session
static int32_t get_account_id(int client_fd) {
    UserSession *session = session_get_by_socket(client_fd);
    if (!session || session->account_id <= 0) {
        return -1;  // Not authenticated
    }
    return session->account_id;
}
```

**Usage in handlers:**
```c
int32_t account_id = get_account_id(client_fd);
if (account_id < 0) {
    send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not authenticated");
    return;
}
```

---

## 📡 NOTIFICATION SYSTEM

### **Broadcast Mechanism**

```c
// room_manager.c
void room_broadcast(int room_id, uint16_t command, 
                   const char *payload, uint32_t payload_len,
                   int exclude_fd) {
    RoomState *room = find_room(room_id);
    if (!room) return;
    
    for (int i = 0; i < room->member_count; i++) {
        int fd = room->members[i].client_fd;
        if (fd != exclude_fd && fd > 0) {
            forward_response(fd, NULL, command, payload, payload_len);
        }
    }
}
```

### **Notification Types**

| Opcode | Name | Purpose | Payload |
|--------|------|---------|---------|
| 700 | `NTF_PLAYER_JOINED` | New member joined | `{"account_id": 5, "username": "Alice"}` |
| 701 | `NTF_PLAYER_LEFT` | Member left | `{"account_id": 5}` |
| 702 | `NTF_PLAYER_LIST` | Full player list update | `[{...}, {...}]` |
| 712 | `NTF_PLAYER_READY` | Ready state changed | `{"account_id": 5, "is_ready": true}` |
| 713 | `NTF_RULES_CHANGED` | Rules updated | `{"mode": "scoring", ...}` |
| 714 | `NTF_MEMBER_KICKED` | Member kicked | `{"account_id": 7}` |
| 715 | `NTF_ROOM_CLOSED` | Room closed by host | `{"success": true}` |

---

## ⚠️ ERROR HANDLING

### **Error Codes**

| Code | Name | Meaning |
|------|------|---------|
| 400 | `ERR_BAD_REQUEST` | Invalid payload |
| 401 | `ERR_NOT_LOGGED_IN` | Session not authenticated |
| 403 | `ERR_ROOM_FULL` | Max players reached |
| 406 | `ERR_NOT_HOST` | Action requires host permission |
| 500 | `ERR_SERVER_ERROR` | Database operation failed |

### **Error Response Format**

```json
{
  "success": false,
  "error": "Human-readable message"
}
```

---

## ✅ TESTING CHECKLIST

### **CREATE ROOM**
- [ ] Room created in DB with correct host_id
- [ ] Room code generated (6 chars)
- [ ] Host added to room_members
- [ ] Host tracked in memory with is_ready=true
- [ ] RES_ROOM_CREATED response correct
- [ ] NTF_RULES_CHANGED broadcast
- [ ] NTF_PLAYER_LIST shows host as ready

### **SET RULES**
- [ ] DB updated with new rules
- [ ] NTF_RULES_CHANGED broadcast to ALL
- [ ] Frontend UI updates for all members
- [ ] is_ready state NOT reset

### **KICK MEMBER**
- [ ] Target removed from DB
- [ ] NTF_MEMBER_KICKED sent
- [ ] Target client redirects to lobby
- [ ] NTF_PLAYER_LIST updated
- [ ] Memory state cleaned up

### **START GAME**
- [ ] Validates all ready
- [ ] Minimum 2 players check
- [ ] Status updated to 'playing'
- [ ] NTF_GAME_START broadcast

---

## 🔄 STATE SYNC STRATEGY

### **Push to DB (Persistent)**
- Room creation
- Rules change
- Member join/leave/kick
- Game start (status change)

### **Memory Only (Volatile)**
- `is_ready` state
- `client_fd` mapping
- Real-time gameplay state

### **Pull from DB (On-demand)**
- Initial room load
- After kick (refresh player list)
- Reconnection scenario

---

## 🐛 KNOWN ISSUES & FIXES

### ✅ **Fixed: Hardcoded account_id**
**Before:**
```c
room_add_member(room_id, client_fd, 1, true);  // ❌ Hardcoded
```

**After:**
```c
int32_t account_id = get_account_id(client_fd);  // ✅ From session
room_add_member(room_id, client_fd, account_id, true);
```

### ✅ **Fixed: Host not auto-ready**
```c
member->is_ready = is_host;  // ✅ Host auto ready
```

### ✅ **Fixed: Round time hardcoded**
```c
#define DEFAULT_ROUND_TIME 15  // ✅ In protocol.h
```

---

## 📝 MERGE CHECKLIST

Before merging to `main`:

- [x] All handlers use `get_account_id(client_fd)`
- [x] Host auto-ready on room creation
- [x] Binary protocol matches C structs
- [x] All broadcasts use room_manager
- [x] Error handling comprehensive
- [x] Memory leaks checked (cJSON cleanup)
- [x] Network byte order used (ntohl/htonl)
- [x] Documentation complete
- [ ] Integration tests passed
- [ ] Code review completed

---

## 🚀 DEPLOYMENT NOTES

### **Environment Variables**
```env
SUPABASE_URL=https://xxx.supabase.co
SUPABASE_KEY=eyJxxx...
SOCKET_SERVER_PORT=5500
WS_BRIDGE_PORT=8080
```

### **Build & Run**
```bash
# Rebuild C server
cd Network
make clean && make

# Start services
docker-compose up --build

# Test
# 1. Login
# 2. Create room
# 3. Change rules
# 4. Open second browser → join room
# 5. Host kicks member
# 6. All ready → start game
```

---

## 📚 REFERENCES

- [APPLICATION DESIGN.txt](./APPLICATION%20DESIGN.txt) - Full protocol spec
- [Workflow.txt](./Workflow.txt) - Development guidelines
- [session rule.txt](./sesion%20rule.txt) - Session management rules
- [schema.sql](../Database/schema.sql) - Database schema

---

**Document Version:** 1.0  
**Last Updated:** January 9, 2026  
**Contributors:** Development Team
