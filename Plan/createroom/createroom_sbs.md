# KẾ HOẠCH TRIỂN KHAI CREATE_ROOM - STEP BY STEP

## 🎯 MỤC TIÊU
Implement đầy đủ use case CREATE_ROOM theo spec `room_usecase.md`, từ đầu đến cuối, có thể test được.

---

## 📋 CHECKLIST TỔNG QUAN

- [ ] **Phase 1:** Sửa Room Manager (foundation)
- [ ] **Phase 2:** Sửa Room Handler (business logic)
- [ ] **Phase 3:** Test compile & fix errors
- [ ] **Phase 4:** Test với hardcoded data
- [ ] **Phase 5:** Test end-to-end với Frontend

**Thời gian ước tính:** 10-15 giờ

---

# PHASE 1: SỬA ROOM MANAGER (2-3 giờ)

## Step 1.1: Sửa RoomState struct

**File:** `Network/include/transport/room_manager.h`

**Nhiệm vụ:** Thay thế struct `RoomState` cũ bằng struct mới đầy đủ

**Code cần thêm:**
```c
// Enums
typedef enum {
    ROOM_WAITING = 0,
    ROOM_PLAYING = 1,
    ROOM_CLOSED = 2
} RoomStatus;

typedef enum {
    MODE_ELIMINATION = 0,
    MODE_SCORING = 1
} GameMode;

typedef enum {
    ROOM_PUBLIC = 0,
    ROOM_PRIVATE = 1
} RoomVisibility;

// Player state
typedef struct {
    uint32_t account_id;
    bool is_host;
    bool is_ready;
    bool connected;
    time_t joined_at;
} RoomPlayerState;

// Room state (THAY THẾ struct cũ)
typedef struct {
    uint32_t id;
    char name[32];
    char code[8];
    uint32_t host_id;
    
    RoomStatus status;
    GameMode mode;
    uint8_t max_players;
    RoomVisibility visibility;
    uint8_t wager_mode;
    
    // Player tracking
    RoomPlayerState players[MAX_ROOM_MEMBERS];
    uint8_t player_count;
    
    // Socket tracking (for broadcast)
    int member_fds[MAX_ROOM_MEMBERS];
    int member_count;
} RoomState;
```

**Checklist:**
- [ ] Thêm enums vào đầu file
- [ ] Thêm `RoomPlayerState` struct
- [ ] Thay thế `RoomState` struct cũ
- [ ] Compile test: `cd Network && make`

---

## Step 1.2: Khai báo API functions

**File:** `Network/include/transport/room_manager.h`

**Nhiệm vụ:** Thêm function declarations

**Code cần thêm:**
```c
// Room lifecycle
RoomState* room_create(void);
void room_destroy(uint32_t room_id);
RoomState* room_get(uint32_t room_id);

// Player management
int room_add_player(uint32_t room_id, uint32_t account_id, int client_fd);
int room_remove_player(uint32_t room_id, uint32_t account_id);
bool room_has_player(uint32_t room_id, uint32_t account_id);

// Existing functions (giữ nguyên)
void room_add_member(int room_id, int client_fd);
void room_remove_member(int room_id, int client_fd);
void room_broadcast(...);
```

**Checklist:**
- [ ] Thêm declarations
- [ ] Compile test

---

## Step 1.3: Implement room_create()

**File:** `Network/src/transport/room_manager.c`

**Code:**
```c
static RoomState g_rooms[MAX_ROOMS];
static int g_room_count = 0;

RoomState* room_create(void) {
    if (g_room_count >= MAX_ROOMS) {
        return NULL;
    }
    
    RoomState *room = &g_rooms[g_room_count++];
    memset(room, 0, sizeof(RoomState));
    
    return room;
}
```

**Checklist:**
- [ ] Thêm global arrays
- [ ] Implement function
- [ ] Compile test

---

## Step 1.4: Implement room_get()

**Code:**
```c
RoomState* room_get(uint32_t room_id) {
    for (int i = 0; i < g_room_count; i++) {
        if (g_rooms[i].id == room_id) {
            return &g_rooms[i];
        }
    }
    return NULL;
}
```

**Checklist:**
- [ ] Implement
- [ ] Compile test

---

## Step 1.5: Implement room_add_player()

**Code:**
```c
int room_add_player(uint32_t room_id, uint32_t account_id, int client_fd) {
    RoomState *room = room_get(room_id);
    if (!room) return -1;
    
    if (room->player_count >= MAX_ROOM_MEMBERS) return -1;
    
    RoomPlayerState *player = &room->players[room->player_count];
    player->account_id = account_id;
    player->is_host = false;
    player->is_ready = false;
    player->connected = true;
    player->joined_at = time(NULL);
    
    room->member_fds[room->player_count] = client_fd;
    room->player_count++;
    room->member_count++;
    
    return 0;
}
```

**Checklist:**
- [ ] Implement
- [ ] Compile test

---

## Step 1.6: Implement room_has_player()

**Code:**
```c
bool room_has_player(uint32_t room_id, uint32_t account_id) {
    RoomState *room = room_get(room_id);
    if (!room) return false;
    
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].account_id == account_id) {
            return true;
        }
    }
    return false;
}
```

**Checklist:**
- [ ] Implement
- [ ] Compile test
- [ ] **PHASE 1 DONE** ✅

---

# PHASE 2: SỬA ROOM HANDLER (3-4 giờ)

## Step 2.1: Sửa CreateRoomPayload struct

**File:** `Network/src/handlers/room_handler.c`

**Nhiệm vụ:** Thay thế struct cũ (72 bytes) bằng struct mới (36 bytes)

**Code:**
```c
typedef struct PACKED {
    char name[32];
    uint8_t mode;
    uint8_t max_players;
    uint8_t visibility;
    uint8_t wager_mode;
} CreateRoomPayload;  // 36 bytes
```

**Checklist:**
- [ ] Xóa struct cũ
- [ ] Thêm struct mới
- [ ] Compile test

---

## Step 2.2: Thêm CreateRoomResponsePayload

**Code:**
```c
typedef struct PACKED {
    uint32_t room_id;      // network byte order
    char room_code[8];
} CreateRoomResponsePayload;  // 12 bytes
```

**Checklist:**
- [ ] Thêm struct
- [ ] Compile test

---

## Step 2.3: Viết lại handle_create_room() - Part 1: Validation

**File:** `Network/src/handlers/room_handler.c`

**Code:**
```c
#include "handlers/session_manager.h"

void handle_create_room(int client_fd, MessageHeader *req, const char *payload) {
    // STEP 1: Session validation
    UserSession *session = session_get_by_socket(client_fd);
    
    if (!session || session->state == SESSION_UNAUTHENTICATED) {
        send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }
    
    if (session->state != SESSION_LOBBY) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid state");
        return;
    }
    
    // STEP 2: Check user not in any room
    extern RoomState g_rooms[];
    extern int g_room_count;
    
    for (int i = 0; i < g_room_count; i++) {
        if (room_has_player(g_rooms[i].id, session->account_id)) {
            send_error(client_fd, req, ERR_BAD_REQUEST, "Already in a room");
            return;
        }
    }
    
    // STEP 3: Validate payload size
    if (req->length != sizeof(CreateRoomPayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload size");
        return;
    }
    
    // Continue in next step...
}
```

**Checklist:**
- [ ] Thêm include session_manager.h
- [ ] Implement validation
- [ ] Compile test

---

## Step 2.4: handle_create_room() - Part 2: Parse & Validate

**Code (tiếp theo):**
```c
    // STEP 4: Parse payload
    CreateRoomPayload data;
    memcpy(&data, payload, sizeof(data));
    
    char room_name[33];
    memcpy(room_name, data.name, 32);
    room_name[32] = '\0';
    
    // STEP 5: Validate business rules
    if (strlen(room_name) == 0 || strlen(room_name) > 32) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid room name");
        return;
    }
    
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
    
    if (data.visibility != ROOM_PUBLIC && data.visibility != ROOM_PRIVATE) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid visibility");
        return;
    }
```

**Checklist:**
- [ ] Implement parsing
- [ ] Implement validation
- [ ] Compile test

---

## Step 2.5: handle_create_room() - Part 3: Create Room

**Code (tiếp theo):**
```c
    // STEP 6: Create RoomState
    RoomState *room = room_create();
    if (!room) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Cannot create room");
        return;
    }
    
    strncpy(room->name, room_name, 32);
    room->host_id = session->account_id;
    room->status = ROOM_WAITING;
    room->mode = data.mode;
    room->max_players = data.max_players;
    room->visibility = data.visibility;
    room->wager_mode = data.wager_mode;
    
    // STEP 7: Call DB to get ID and code
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
        room_destroy(room->id);
        send_error(client_fd, req, ERR_SERVER_ERROR, "DB error");
        return;
    }
    
    // Update room with DB values
    room->id = db_room_id;
    strncpy(room->code, db_room_code, 8);
```

**Checklist:**
- [ ] Implement room creation
- [ ] Implement DB call
- [ ] Compile test

---

## Step 2.6: handle_create_room() - Part 4: Response

**Code (tiếp theo):**
```c
    // STEP 8: Add host as player
    room_add_player(room->id, session->account_id, client_fd);
    room->players[0].is_host = true;
    
    // STEP 9: Send response
    CreateRoomResponsePayload resp;
    resp.room_id = htonl(room->id);
    memcpy(resp.room_code, room->code, 8);
    
    forward_response(client_fd, req, RES_ROOM_CREATED, 
                     (char*)&resp, sizeof(resp));
    
    // STEP 10: Broadcast NTF_PLAYER_LIST
    // TODO: Implement in next phase
}
```

**Checklist:**
- [ ] Implement response
- [ ] Compile test
- [ ] **PHASE 2 DONE** ✅

---

# PHASE 3: TEST COMPILE (30 phút)

## Step 3.1: Fix compile errors

**Checklist:**
- [ ] `cd Network && make clean && make`
- [ ] Fix all errors
- [ ] Fix all warnings

---

# PHASE 4: TEST VỚI HARDCODED DATA (1 giờ)

## Step 4.1: Tạo test script

**File:** `Network/test_create_room.c`

**Code:**
```c
#include "handlers/room_handler.h"

int main() {
    // Mock session
    UserSession session;
    session.account_id = 123;
    session.state = SESSION_LOBBY;
    
    // Mock payload
    CreateRoomPayload payload;
    strcpy(payload.name, "Test Room");
    payload.mode = MODE_SCORING;
    payload.max_players = 4;
    payload.visibility = ROOM_PUBLIC;
    payload.wager_mode = 0;
    
    // Mock header
    MessageHeader header;
    header.length = sizeof(payload);
    
    // Call handler
    handle_create_room(1, &header, (char*)&payload);
    
    return 0;
}
```

**Checklist:**
- [ ] Tạo test file
- [ ] Compile: `gcc test_create_room.c -o test`
- [ ] Run: `./test`
- [ ] Check console output

---

# PHASE 5: TEST END-TO-END (2 giờ)

## Step 5.1: Start services

**Checklist:**
- [ ] Start C Server: `cd Network && ./server`
- [ ] Start ws-bridge: `cd ws-bridge && node index.js`
- [ ] Start Frontend: `cd Frontend && npm start`

---

## Step 5.2: Test từ UI

**Checklist:**
- [ ] Mở browser: `http://localhost:3000`
- [ ] Click "Create Room"
- [ ] Fill form và submit
- [ ] Check console logs
- [ ] Verify response

---

## Step 5.3: Debug nếu lỗi

**Tools:**
- [ ] Check C Server logs
- [ ] Check ws-bridge logs
- [ ] Check Frontend console
- [ ] Use Wireshark nếu cần

---

# ✅ HOÀN THÀNH

Khi tất cả checklist đã tick ✅, use case CREATE_ROOM đã hoàn thiện!

**Next steps:**
- Implement JOIN_ROOM
- Implement READY
- Implement SET_RULES
