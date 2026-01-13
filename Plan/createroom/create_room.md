# KẾ HOẠCH TRIỂN KHAI CREATE_ROOM - PHIÊN BẢN CUỐI CÙNG

## I. NGUYÊN TẮC THIẾT KẾ

### Session State Logic (ĐÃ CHỐT)

```
SESSION_LOBBY = User KHÔNG ở trong bất kỳ room nào
```

**Validation đơn giản:**
1. Check `session->state == SESSION_LOBBY` → OK để create/join room
2. Iterate qua `g_rooms` để check `account_id` có trong [players](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/handlers/round1_handler.c#201-213) không
3. Nếu đã ở room → `ERR_BAD_REQUEST`
4. Disconnect → cleanup room membership → về lobby

**❌ KHÔNG CẦN:** Mapping `account_id -> room_id` phức tạp

---

## II. CẤU TRÚC CODE & TỔ CHỨC MODULE

### A. Room Manager (Transport Layer)

**File:** [Network/include/transport/room_manager.h](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/include/transport/room_manager.h)  
**File:** [Network/src/transport/room_manager.c](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c)

**Trách nhiệm:** Quản lý in-memory room state và broadcast

```c
// STRUCT CHÍNH (sửa lại hoàn toàn)
typedef struct {
    uint32_t id;
    char name[32];
    char code[8];
    uint32_t host_id;
    
    enum RoomStatus {
        ROOM_WAITING,
        ROOM_PLAYING,
        ROOM_CLOSED
    } status;
    
    enum GameMode {
        MODE_ELIMINATION = 0,
        MODE_SCORING = 1
    } mode;
    
    uint8_t max_players;
    
    enum RoomVisibility {
        ROOM_PUBLIC = 0,
        ROOM_PRIVATE = 1
    } visibility;
    
    uint8_t wager_mode;  // 0 = OFF, 1 = ON
    
    // Player tracking
    RoomPlayerState players[MAX_ROOM_MEMBERS];
    uint8_t player_count;
} RoomState;

typedef struct {
    uint32_t account_id;
    bool is_host;
    bool is_ready;
    bool connected;
    time_t joined_at;
} RoomPlayerState;
```

**API Functions:**
```c
// Room lifecycle
RoomState* room_create(void);
void room_destroy(uint32_t room_id);
RoomState* room_get(uint32_t room_id);

// Player management
int room_add_player(uint32_t room_id, uint32_t account_id, int client_fd);
int room_remove_player(uint32_t room_id, uint32_t account_id);
bool room_has_player(uint32_t room_id, uint32_t account_id);

// Broadcast (giữ nguyên)
void room_broadcast(int room_id, uint16_t command, const char *payload, 
                   uint32_t payload_len, int exclude_fd);
```

📌 **LƯU Ý:** `room_id` và `room_code` được generate bởi **DATABASE**, không phải in-memory.

---

### B. Room Handler (Application Layer)

**File:** [Network/src/handlers/room_handler.c](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/handlers/room_handler.c)

**Trách nhiệm:** Business logic cho room use cases

**Payload Structs (sửa lại):**
```c
typedef struct PACKED {
    char name[32];
    uint8_t mode;
    uint8_t max_players;
    uint8_t visibility;
    uint8_t wager_mode;
} CreateRoomPayload;  // 36 bytes

typedef struct PACKED {
    uint32_t room_id;      // network byte order
    char room_code[8];
} CreateRoomResponsePayload;  // 12 bytes
```

**Handler Function:**
```c
void handle_create_room(int client_fd, MessageHeader *req, const char *payload);
```

---

### C. Room Repository (Database Layer)

**File:** `Network/src/db/repo/room_repo.c`

**Trách nhiệm:** Persistence ONLY, không có logic game

**Sửa signature:**
```c
int room_repo_create(
    const char *name,
    uint8_t visibility,
    uint8_t mode,
    uint8_t max_players,
    uint8_t wager_mode,
    char *out_room_code,      // OUTPUT: room code
    size_t code_size,
    uint32_t *out_room_id     // OUTPUT: room id
);
```

**Return:** 
- `0` = success
- `-1` = error

---

## III. FLOW XỬ LÝ CHI TIẾT

### Step 1: Validate Session (CRITICAL)

```c
#include "handlers/session_manager.h"

UserSession *session = session_get_by_socket(client_fd);

if (!session || session->state == SESSION_UNAUTHENTICATED) {
    send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not logged in");
    return;
}

if (session->state != SESSION_LOBBY) {
    send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid state");
    return;
}
```

---

### Step 2: Check User Not In Any Room

```c
// Iterate qua global rooms
extern RoomState g_rooms[MAX_ROOMS];
extern int g_room_count;

for (int i = 0; i < g_room_count; i++) {
    if (room_has_player(g_rooms[i].id, session->account_id)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, 
                   "Already in a room");
        return;
    }
}
```

---

### Step 3: Parse & Validate Payload

```c
if (req->length != sizeof(CreateRoomPayload)) {
    send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload size");
    return;
}

CreateRoomPayload data;
memcpy(&data, payload, sizeof(data));

// Null-terminate name
char room_name[33];
memcpy(room_name, data.name, 32);
room_name[32] = '\0';

// Validate name
if (strlen(room_name) == 0 || strlen(room_name) > 32) {
    send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid room name");
    return;
}

// Validate mode vs max_players
if (data.mode == MODE_ELIMINATION) {
    if (data.max_players != 4) {
        send_error(client_fd, req, ERR_BAD_REQUEST, 
                   "ELIMINATION requires exactly 4 players");
        return;
    }
} else if (data.mode == MODE_SCORING) {
    if (data.max_players < 4 || data.max_players > 6) {
        send_error(client_fd, req, ERR_BAD_REQUEST, 
                   "SCORING requires 4-6 players");
        return;
    }
} else {
    send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid mode");
    return;
}

// Validate visibility
if (data.visibility != ROOM_PUBLIC && data.visibility != ROOM_PRIVATE) {
    send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid visibility");
    return;
}

// Validate wager_mode
if (data.wager_mode != 0 && data.wager_mode != 1) {
    send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid wager mode");
    return;
}
```

---

### Step 4: Create RoomState (In-Memory) - CHƯA CÓ ID/CODE

```c
RoomState *room = room_create();
if (!room) {
    send_error(client_fd, req, ERR_SERVER_ERROR, "Cannot create room");
    return;
}

// Set properties (KHÔNG set id và code)
strncpy(room->name, room_name, 32);
room->host_id = session->account_id;
room->status = ROOM_WAITING;
room->mode = data.mode;
room->max_players = data.max_players;
room->visibility = data.visibility;
room->wager_mode = data.wager_mode;

// id và code sẽ được set sau khi DB trả về
```

---

### Step 5: Add Host as First Player

```c
room_add_player(room->id, session->account_id, client_fd);

// Set host flag
for (int i = 0; i < room->player_count; i++) {
    if (room->players[i].account_id == session->account_id) {
        room->players[i].is_host = true;
        room->players[i].is_ready = false;
        room->players[i].connected = true;
        room->players[i].joined_at = time(NULL);
        break;
    }
}
```

---

### Step 6: Persist to Database

```c
char db_room_code[9] = {0};
uint32_t db_room_id = 0;

int rc = room_repo_create(
    room_name,
    data.visibility,
    data.mode,
    data.max_players,
    data.wager_mode,
    db_room_code,
    sizeof(db_room_code),
    &db_room_id
);

if (rc != 0) {
    // Rollback in-memory state
    room_destroy(room->id);
    send_error(client_fd, req, ERR_SERVER_ERROR, "DB error");
    return;
}

// Update room with DB values (if different)
room->id = db_room_id;
strncpy(room->code, db_room_code, 8);
```

---

### Step 7: Send Response

```c
CreateRoomResponsePayload resp;
resp.room_id = htonl(room->id);
memcpy(resp.room_code, room->code, 8);

forward_response(client_fd, req, RES_ROOM_CREATED, 
                 (char*)&resp, sizeof(resp));
```

---

### Step 8: Broadcast NTF_PLAYER_LIST

```c
// Build player list notification
typedef struct PACKED {
    uint32_t room_id;
    uint8_t player_count;
    uint8_t reserved;
} PlayerListHeader;

typedef struct PACKED {
    uint32_t account_id;
    char username[32];
    uint8_t is_host;
    uint8_t is_ready;
    uint8_t connected;
} PlayerInfo;

char notif_buf[256];
PlayerListHeader *header = (PlayerListHeader*)notif_buf;
header->room_id = htonl(room->id);
header->player_count = room->player_count;
header->reserved = 0;

// Add player info (chỉ có host)
PlayerInfo *info = (PlayerInfo*)(notif_buf + sizeof(PlayerListHeader));
info->account_id = htonl(session->account_id);
// TODO: Get username from session or DB
strncpy(info->username, "Host", 32);
info->is_host = 1;
info->is_ready = 0;
info->connected = 1;

uint32_t notif_len = sizeof(PlayerListHeader) + sizeof(PlayerInfo);

room_broadcast(room->id, NTF_PLAYER_LIST, notif_buf, notif_len, -1);
```

---

## IV. CHECKLIST TRIỂN KHAI

### Phase 1: Sửa Room Manager (2-3 giờ)
- [ ] Sửa `RoomState` struct trong [room_manager.h](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/include/transport/room_manager.h)
- [ ] Thêm `RoomPlayerState` struct
- [ ] Implement `room_create()`, `room_destroy()`, [room_get()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c#177-180)
- [ ] Implement `room_add_player()`, `room_has_player()`
- [ ] Implement `room_generate_id()`, `room_generate_code()`
- [ ] Test compile

### Phase 2: Sửa Room Handler (3-4 giờ)
- [ ] Sửa `CreateRoomPayload` struct (36 bytes)
- [ ] Thêm `CreateRoomResponsePayload` struct (12 bytes)
- [ ] Implement session validation
- [ ] Implement "already in room" check
- [ ] Implement payload validation (mode, max_players, etc.)
- [ ] Test compile

### Phase 3: Integrate Flow (2-3 giờ)
- [ ] Create RoomState in-memory
- [ ] Add host as player
- [ ] Call `room_repo_create()`
- [ ] Handle DB errors + rollback
- [ ] Send binary response
- [ ] Test với hardcoded data

### Phase 4: Implement Notification (1-2 giờ)
- [ ] Build `NTF_PLAYER_LIST` payload
- [ ] Call [room_broadcast()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c#127-165)
- [ ] Test với console log

### Phase 5: Testing (2-3 giờ)
- [ ] Test với 1 user create room
- [ ] Test validation errors
- [ ] Test DB persistence
- [ ] Test với Frontend
- [ ] Check memory leaks

**TỔNG THỜI GIAN:** 10-15 giờ

---

## V. KẾT LUẬN

**Cấu trúc code:**
```
room_manager.h/c    → In-memory state + broadcast (Transport)
room_handler.c      → Business logic (Application)
room_repo.c         → DB persistence (Data)
session_manager.h   → Session validation
```

**Ưu tiên:**
1. Sửa [room_manager.h](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/include/transport/room_manager.h) trước (foundation)
2. Sửa payload structs
3. Implement validation logic
4. Integrate DB + notification

**Không cần:**
- ❌ Mapping `account_id -> room_id` phức tạp
- ❌ Query DB để check realtime
- ❌ JSON response (dùng binary)
