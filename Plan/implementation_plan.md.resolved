# Kế Hoạch Triển Khai: JOIN_ROOM Use Case

## I. TỔNG QUAN

### Mục Tiêu
Triển khai use case JOIN_ROOM cho phép người chơi tham gia phòng chờ hiện có qua:
1. **Danh sách công khai** (join bằng room_id)
2. **Mã phòng riêng tư** (join bằng room_code)

### Tiêu Chí Thành Công
- ✅ Người chơi có thể join phòng công khai từ danh sách lobby
- ✅ Người chơi có thể join phòng riêng tư bằng mã 6 ký tự
- ✅ Server validate đầy đủ capacity và trạng thái phòng
- ✅ Tên người chơi được lấy từ DB và cache vào memory
- ✅ Tất cả người chơi nhận được thông báo NTF_PLAYER_LIST
- ✅ Frontend WaitingRoom hiển thị người chơi mới

---

## II. GIAO THỨC MẠNG

### Lệnh: CMD_JOIN_ROOM (0x0201)

**Cấu trúc Payload (16 bytes cố định):**
```c
typedef struct PACKED {
    uint8_t  by_code;      // 0 = join bằng room_id, 1 = join bằng room_code
    uint8_t  reserved[3];  // padding để alignment
    uint32_t room_id;      // network byte order - dùng nếu by_code = 0
    char     room_code[8]; // null-terminated - dùng nếu by_code = 1
} JoinRoomPayload;
```

**Lý do thiết kế:**
- Payload cố định 16 bytes → parsing đơn giản
- Server chỉ cần check [by_code](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/db/repo/room_repo.c#248-289) để biết đọc field nào
- Alignment tốt (4-byte boundaries)

### Phản hồi: RES_ROOM_JOINED (0x00DD)

**Payload:** Rỗng (0 bytes)

Thành công được thể hiện qua opcode.

### Thông báo (Broadcast)

1. **NTF_PLAYER_JOINED (0x02BC)** - Gửi cho tất cả người chơi hiện tại
2. **NTF_PLAYER_LIST (0x02BE)** - Gửi cho tất cả (kể cả người mới join)

---

## III. TRIỂN KHAI SERVER-SIDE

### Cấu Trúc File

```
Network/
├── src/handlers/room_handler.c
│   └── handle_join_room()          [HÀM MỚI]
├── src/transport/room_manager.c
│   ├── room_add_player()           [ĐÃ CÓ - tái sử dụng]
│   ├── broadcast_player_list()     [ĐÃ CÓ - tái sử dụng]
│   └── find_room_by_code()         [HÀM MỚI]
└── include/protocol/opcode.h
    ├── CMD_JOIN_ROOM (0x0201)      [KIỂM TRA ĐÃ CÓ]
    ├── RES_ROOM_JOINED (0x00DD)    [KIỂM TRA ĐÃ CÓ]
    └── NTF_PLAYER_JOINED (0x02BC)  [KIỂM TRA ĐÃ CÓ]
```

---

### Triển Khai Từng Bước

#### BƯỚC 1: Validate Session & State

```c
void handle_join_room(int client_fd, MessageHeader *req, const char *payload) {
    // 1.1 Lấy session
    UserSession *session = session_get_by_socket(client_fd);
    
    if (!session || session->state == SESSION_UNAUTHENTICATED) {
        send_error(client_fd, req, ERR_NOT_LOGGED_IN, "Not logged in");
        return;
    }
    
    if (session->state != SESSION_LOBBY) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid state");
        return;
    }
    
    // 1.2 Kiểm tra user chưa ở trong phòng nào
    extern RoomState g_rooms[];
    extern int g_room_count;
    
    for (int i = 0; i < g_room_count; i++) {
        if (room_has_player(g_rooms[i].id, session->account_id)) {
            send_error(client_fd, req, ERR_BAD_REQUEST, "Already in a room");
            return;
        }
    }
}
```

---

#### BƯỚC 2: Parse & Validate Payload

```c
    // 2.1 Validate kích thước payload
    if (req->length != sizeof(JoinRoomPayload)) {
        send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid payload size");
        return;
    }
    
    // 2.2 Parse payload
    JoinRoomPayload data;
    memcpy(&data, payload, sizeof(data));
    
    // 2.3 Chuyển đổi network byte order
    uint32_t target_room_id = ntohl(data.room_id);
    
    printf("[SERVER] [JOIN_ROOM] Yêu cầu từ fd=%d, by_code=%d\n", 
           client_fd, data.by_code);
```

---

#### BƯỚC 3: Tìm Phòng

```c
    // 3.1 Tìm phòng dựa trên phương thức join
    RoomState *room = NULL;
    
    if (data.by_code == 0) {
        // Join bằng room_id (từ danh sách công khai)
        room = room_get(target_room_id);
        
        if (!room) {
            send_error(client_fd, req, ERR_BAD_REQUEST, "Room not found");
            return;
        }
        
        // QUAN TRỌNG: Join từ danh sách yêu cầu phòng CÔNG KHAI
        if (room->visibility != ROOM_PUBLIC) {
            send_error(client_fd, req, ERR_BAD_REQUEST, "Room is private");
            return;
        }
        
    } else {
        // Join bằng room_code (nhập mã riêng tư)
        // Null-terminate code
        char room_code[9];
        memcpy(room_code, data.room_code, 8);
        room_code[8] = '\0';
        
        // Tìm theo code
        room = find_room_by_code(room_code);
        
        if (!room) {
            send_error(client_fd, req, ERR_BAD_REQUEST, "Invalid room code");
            return;
        }
        
        // Phòng riêng tư có thể join bằng code bất kể visibility
    }
```

**Lưu ý:** Cần implement hàm helper `find_room_by_code()`.

---

#### BƯỚC 4: Validate Trạng Thái & Sức Chứa Phòng

```c
    // 4.1 Kiểm tra trạng thái phòng
    if (room->status != ROOM_WAITING) {
        send_error(client_fd, req, ERR_GAME_STARTED, "Game already started");
        return;
    }
    
    // 4.2 Kiểm tra sức chứa dựa trên mode
    if (room->mode == MODE_ELIMINATION) {
        if (room->player_count >= 4) {
            send_error(client_fd, req, ERR_ROOM_FULL, "Room is full");
            return;
        }
    } else { // MODE_SCORING
        if (room->player_count >= room->max_players) {
            send_error(client_fd, req, ERR_ROOM_FULL, "Room is full");
            return;
        }
    }
    
    printf("[SERVER] [JOIN_ROOM] Phòng %u (%s) có %d/%d người chơi\n",
           room->id, room->code, room->player_count, room->max_players);
```

---

#### BƯỚC 5: Lấy Tên Người Chơi Từ DB

```c
    // 5.1 Query bảng profiles
    char profile_name[64] = "Player";  // fallback mặc định
    char query[128];
    snprintf(query, sizeof(query), "account_id=eq.%u", session->account_id);
    
    cJSON *profile_response = NULL;
    db_error_t profile_rc = db_get("profiles", query, &profile_response);
    
    if (profile_rc == DB_OK && profile_response) {
        cJSON *first = cJSON_GetArrayItem(profile_response, 0);
        if (first) {
            cJSON *name_item = cJSON_GetObjectItem(first, "name");
            if (name_item && cJSON_IsString(name_item)) {
                strncpy(profile_name, name_item->valuestring, sizeof(profile_name) - 1);
                profile_name[sizeof(profile_name) - 1] = '\0';
            }
        }
        cJSON_Delete(profile_response);
    }
    
    printf("[SERVER] [JOIN_ROOM] Tên người chơi: %s\n", profile_name);
```

---

#### BƯỚC 6: Thêm Người Chơi Vào Phòng

```c
    // 6.1 Thêm vào room state trong memory
    int rc = room_add_player(room->id, session->account_id, profile_name, client_fd);
    
    if (rc != 0) {
        send_error(client_fd, req, ERR_SERVER_ERROR, "Failed to add player");
        return;
    }
    
    printf("[SERVER] [JOIN_ROOM] Người chơi %u (%s) đã join phòng %u\n",
           session->account_id, profile_name, room->id);
```

---

#### BƯỚC 7: Lưu Vào Database

```c
    // 7.1 Insert vào bảng room_members
    // 
    // 📌 DESIGN DECISION: DB Persistence Strategy
    // ==========================================
    // - In-memory state (RoomState) là AUTHORITATIVE cho realtime gameplay
    // - Database chỉ dùng cho:
    //   * Persistence (recovery sau khi server restart)
    //   * Analytics/reporting
    //   * Audit trail
    // 
    // - Nếu DB insert fail:
    //   ✅ Game vẫn tiếp tục (best-effort approach)
    //   ✅ Player đã được thêm vào memory → có thể chơi ngay
    //   ⚠️ Trade-off: Ưu tiên UX > data consistency tức thời
    // 
    // - Eventual consistency:
    //   * DB sẽ được sync lại khi có cơ hội
    //   * Hoặc cleanup khi server shutdown gracefully
    
    cJSON *member_payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(member_payload, "room_id", room->id);
    cJSON_AddNumberToObject(member_payload, "account_id", session->account_id);
    
    cJSON *db_response = NULL;
    db_error_t db_rc = db_post("room_members", member_payload, &db_response);
    cJSON_Delete(member_payload);
    
    if (db_rc != DB_OK) {
        printf("[SERVER] [JOIN_ROOM] ⚠️ DB insert failed (non-critical)\n");
        printf("[SERVER] [JOIN_ROOM] Player already in memory, game can proceed\n");
        // Continue anyway - eventual consistency model
    }
    
    if (db_response) {
        cJSON_Delete(db_response);
    }
```

---

#### BƯỚC 8: Gửi Phản Hồi Cho Người Join

```c
    // 8.1 Gửi RES_ROOM_JOINED (payload rỗng)
    forward_response(client_fd, req, RES_ROOM_JOINED, NULL, 0);
    
    printf("[SERVER] [JOIN_ROOM] ✅ THÀNH CÔNG: người chơi %u join phòng %u\n",
           session->account_id, room->id);
```

---

#### BƯỚC 9: Broadcast Thông Báo

```c
    // 9.1 Tạo thông báo NTF_PLAYER_JOINED
    char joined_notif[256];
    int offset = snprintf(joined_notif, sizeof(joined_notif),
        "{\"account_id\":%u,\"name\":\"%s\"}",
        session->account_id, profile_name);
    
    // Broadcast cho tất cả TRỪ người vừa join
    room_broadcast(room->id, NTF_PLAYER_JOINED, joined_notif, offset, client_fd);
    
    // 9.2 Broadcast danh sách người chơi cập nhật cho TẤT CẢ (kể cả người join)
    broadcast_player_list(room->id);
```

---

### Hàm Helper: find_room_by_code()

**Vị trí:** [Network/src/transport/room_manager.c](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c)

```c
RoomState* find_room_by_code(const char *code) {
    extern RoomState g_rooms[];
    extern int g_room_count;
    
    for (int i = 0; i < g_room_count; i++) {
        if (strncmp(g_rooms[i].code, code, 8) == 0) {
            return &g_rooms[i];
        }
    }
    return NULL;
}
```

**Khai báo trong header:** Thêm vào [room_manager.h](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/include/transport/room_manager.h)

---

### Cải Tiến: Race Condition Protection trong room_add_player()

**Vấn đề:** Khi 2 clients join gần như đồng thời, cả hai có thể pass capacity check trước khi `player_count` được tăng.

**Giải pháp:** Re-check capacity bên trong [room_add_player()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c#65-88)

**Cập nhật [room_add_player()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c#65-88) trong [room_manager.c](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c):**

```c
int room_add_player(uint32_t room_id, uint32_t account_id, const char *name, int client_fd) {
    RoomState *room = find_room(room_id);
    if (!room) return -1;
    
    // 🔒 CRITICAL SECTION: Re-check capacity
    // Bảo vệ khỏi race condition khi nhiều clients join đồng thời
    if (room->player_count >= MAX_ROOM_MEMBERS) {
        printf("[ROOM] ⚠️ Race condition detected: room already full\n");
        return -1;
    }
    
    // Proceed with adding player...
    RoomPlayerState *player = &room->players[room->player_count];
    player->account_id = account_id;
    strncpy(player->name, name ? name : "Player", sizeof(player->name) - 1);
    player->name[sizeof(player->name) - 1] = '\0';
    player->is_host = false;
    player->is_ready = false;
    player->connected = true;
    player->joined_at = time(NULL);
    
    room->member_fds[room->player_count] = client_fd;
    room->player_count++;
    room->member_count++;
    
    printf("[ROOM] Added player: id=%u, name='%s' to room %u\n", 
           account_id, player->name, room_id);
    
    return 0;
}
```

**Ghi chú cho Report:**
> "Để xử lý race condition khi nhiều clients join đồng thời, capacity được kiểm tra lại trong [room_add_player()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c#65-88). Giải pháp này đủ cho game scale nhỏ-trung bình. Với production scale lớn hơn, có thể cần mutex hoặc atomic operations."

---

## IV. TRIỂN KHAI FRONTEND

### Cấu Trúc File

```
Frontend/src/
├── services/
│   └── roomService.js
│       └── joinRoom(roomId, roomCode)  [HÀM MỚI]
├── components/Lobby/
│   ├── RoomList.js
│   │   └── handleJoinClick()           [CẬP NHẬT]
│   └── JoinByCodeModal.js              [COMPONENT MỚI]
└── components/Room/WaitingRoom/
    └── WaitingRoom.js
        └── NTF_PLAYER_JOINED handler   [THÊM]
```

---

### Bước 1: Tạo roomService.js

**File:** [Frontend/src/services/roomService.js](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Frontend/src/services/roomService.js)

```javascript
import { sendPacket } from '../network/dispatcher';
import { OPCODE } from '../network/opcode';

/**
 * Join phòng bằng ID hoặc code
 * @param {number|null} roomId - Room ID (cho join từ danh sách công khai)
 * @param {string|null} roomCode - Room code (cho join riêng tư)
 */
export async function joinRoom(roomId = null, roomCode = null) {
    console.log('[ROOM_SERVICE] joinRoom:', { roomId, roomCode });
    
    // Xác định phương thức join
    const byCode = roomCode ? 1 : 0;
    
    // Tạo payload 16-byte
    const buffer = new ArrayBuffer(16);
    const view = new DataView(buffer);
    
    // byte 0: by_code
    view.setUint8(0, byCode);
    
    // bytes 1-3: reserved (padding)
    view.setUint8(1, 0);
    view.setUint8(2, 0);
    view.setUint8(3, 0);
    
    // bytes 4-7: room_id (network byte order)
    if (byCode === 0 && roomId) {
        view.setUint32(4, roomId, false); // big-endian
    } else {
        view.setUint32(4, 0, false);
    }
    
    // bytes 8-15: room_code (8 bytes, null-terminated)
    if (byCode === 1 && roomCode) {
        const encoder = new TextEncoder();
        const codeBytes = encoder.encode(roomCode.substring(0, 8));
        const codeArray = new Uint8Array(buffer, 8, 8);
        
        // 🔧 FIX: Clear buffer trước để tránh dữ liệu rác từ lần trước
        // Ví dụ bug: Join "ABC123" rồi join "XY" → buffer còn "XYC123"
        codeArray.fill(0);
        codeArray.set(codeBytes);
    }
    
    // Gửi packet
    sendPacket(OPCODE.CMD_JOIN_ROOM, new Uint8Array(buffer));
}
```

---

### Bước 2: Cập Nhật RoomList.js

**Thêm handler cho nút join:**

```javascript
import { joinRoom } from '../../services/roomService';
import { useNavigate } from 'react-router-dom';

const handleJoinRoom = async (room) => {
    console.log('Đang join phòng:', room.id);
    
    try {
        await joinRoom(room.id, null); // Join bằng ID
        
        // Đợi phản hồi RES_ROOM_JOINED
        // (được xử lý bởi roomService listener)
        
    } catch (error) {
        console.error('Join phòng thất bại:', error);
        alert('Không thể join phòng');
    }
};
```

---

### Bước 3: Tạo JoinByCodeModal.js

**Component mới để nhập mã phòng riêng tư:**

```javascript
import { useState } from 'react';
import { joinRoom } from '../../services/roomService';

export default function JoinByCodeModal({ onClose }) {
    const [code, setCode] = useState('');
    
    const handleSubmit = async (e) => {
        e.preventDefault();
        
        if (code.length !== 6) {
            alert('Mã phòng phải có 6 ký tự');
            return;
        }
        
        try {
            await joinRoom(null, code.toUpperCase());
            onClose();
        } catch (error) {
            alert('Mã phòng không hợp lệ');
        }
    };
    
    return (
        <div className="modal">
            <form onSubmit={handleSubmit}>
                <h2>Join Phòng Riêng Tư</h2>
                <input
                    type="text"
                    value={code}
                    onChange={(e) => setCode(e.target.value)}
                    placeholder="Nhập mã 6 ký tự"
                    maxLength={6}
                />
                <button type="submit">Join</button>
                <button type="button" onClick={onClose}>Hủy</button>
            </form>
        </div>
    );
}
```

---

### Bước 4: Thêm Response Handler trong roomService.js

```javascript
import { registerHandler } from '../network/receiver';

// Đăng ký handler cho RES_ROOM_JOINED
registerHandler(OPCODE.RES_ROOM_JOINED, (payload) => {
    console.log('[ROOM_SERVICE] ✅ Join phòng thành công');
    
    // Dispatch custom event để điều hướng
    window.dispatchEvent(new CustomEvent('room_joined', {
        detail: { success: true }
    }));
});

// Đăng ký handler cho lỗi
registerHandler(OPCODE.ERR_ROOM_FULL, (payload) => {
    const text = new TextDecoder().decode(payload);
    alert(`Phòng đã đầy: ${text}`);
});

registerHandler(OPCODE.ERR_GAME_STARTED, (payload) => {
    const text = new TextDecoder().decode(payload);
    alert(`Game đã bắt đầu: ${text}`);
});
```

---

### Bước 5: Cập Nhật WaitingRoom.js

**Thêm handler NTF_PLAYER_JOINED:**

```javascript
// Trong useEffect nơi các handler khác được đăng ký

registerHandler(OPCODE.NTF_PLAYER_JOINED, (payload) => {
    const text = new TextDecoder().decode(payload);
    console.log('[WaitingRoom] Người chơi mới join:', text);
    
    try {
        const newPlayer = JSON.parse(text);
        console.log('[WaitingRoom] Người chơi mới:', newPlayer);
        
        // Lưu ý: NTF_PLAYER_LIST sẽ theo ngay sau với danh sách đầy đủ
        // Nên không cần thêm thủ công ở đây
        
    } catch (e) {
        console.error('[WaitingRoom] Parse player joined thất bại:', e);
    }
});
```

**Lưu ý:** Handler `NTF_PLAYER_LIST` (đã implement) sẽ cập nhật danh sách đầy đủ, nên `NTF_PLAYER_JOINED` chủ yếu dùng cho logging/animations.

---

## V. XỬ LÝ LỖI

### Mã Lỗi

| Mã | Opcode | Điều Kiện |
|------|--------|-----------|
| ERR_NOT_LOGGED_IN | 0x0191 | User chưa đăng nhập |
| ERR_BAD_REQUEST | 0x0190 | Đã ở trong phòng / Payload không hợp lệ |
| ERR_ROOM_FULL | 0x0193 | Phòng đã đầy |
| ERR_GAME_STARTED | 0x0194 | Trạng thái phòng != WAITING |

### Hiển Thị Lỗi Frontend

```javascript
registerHandler(OPCODE.ERR_BAD_REQUEST, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    alert(`Lỗi: ${data.error || 'Yêu cầu không hợp lệ'}`);
});
```

---

## VI. CHECKLIST TESTING

### Test Server

- [ ] Join phòng công khai bằng ID (happy path)
- [ ] Join phòng riêng tư bằng code (happy path)
- [ ] Từ chối: User đã ở trong phòng
- [ ] Từ chối: Phòng đầy (elimination = 4, scoring = max_players)
- [ ] Từ chối: Game đã bắt đầu
- [ ] Từ chối: Phòng riêng tư qua danh sách công khai
- [ ] Từ chối: Mã phòng không hợp lệ
- [ ] Xác minh: Tên người chơi lấy từ DB
- [ ] Xác minh: Insert vào room_members DB
- [ ] Xác minh: Broadcast NTF_PLAYER_JOINED
- [ ] Xác minh: Broadcast NTF_PLAYER_LIST

### Test Frontend

- [ ] Join từ danh sách phòng công khai
- [ ] Join qua modal nhập mã
- [ ] Điều hướng đến WaitingRoom sau khi join
- [ ] Hiển thị tất cả người chơi kể cả người mới
- [ ] Hiển thị thông báo lỗi khi thất bại
- [ ] Xử lý nhiều người join cùng lúc

---

## VII. THỨ TỰ TRIỂN KHAI

1. **Server - Logic Cốt Lõi** (2-3 giờ)
   - [ ] Implement `handle_join_room()` trong [room_handler.c](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/handlers/room_handler.c)
   - [ ] Thêm helper `find_room_by_code()`
   - [ ] Test với payload hardcoded

2. **Server - Tích Hợp** (1 giờ)
   - [ ] Kết nối với dispatcher
   - [ ] Test với DB thật
   - [ ] Xác minh broadcasts

3. **Frontend - Service Layer** (1 giờ)
   - [ ] Tạo [roomService.js](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Frontend/src/services/roomService.js)
   - [ ] Implement hàm `joinRoom()`
   - [ ] Thêm response handlers

4. **Frontend - UI Components** (2 giờ)
   - [ ] Cập nhật RoomList với nút join
   - [ ] Tạo JoinByCodeModal
   - [ ] Thêm logic điều hướng

5. **Frontend - WaitingRoom** (30 phút)
   - [ ] Thêm handler NTF_PLAYER_JOINED
   - [ ] Test cập nhật danh sách người chơi

6. **Testing End-to-End** (1-2 giờ)
   - [ ] Test tất cả kịch bản
   - [ ] Sửa bugs
   - [ ] Xác minh edge cases

**Tổng Thời Gian Ước Tính:** 7-9 giờ

---

## VIII. PHỤ THUỘC

### Code Hiện Có (Tái Sử Dụng)
- ✅ [room_add_player()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c#65-88) - Đã implement
- ✅ [broadcast_player_list()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c#262-302) - Đã implement
- ✅ [room_get()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c#57-60) - Đã implement
- ✅ [room_has_player()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c#108-119) - Đã implement

### Code Mới Cần Viết
- ❌ `find_room_by_code()` - Cần implement
- ❌ `handle_join_room()` - Cần implement
- ❌ [roomService.js](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Frontend/src/services/roomService.js) - Cần tạo
- ❌ `JoinByCodeModal.js` - Cần tạo

---

## IX. DESIGN DECISIONS & TRADE-OFFS (CHO REPORT)

### 1. DB Eventual Consistency

**Decision:** In-memory state là authoritative, DB là best-effort

**Rationale:**
- Realtime game cần response time < 100ms
- DB query có thể mất 50-200ms
- Nếu chờ DB confirm → UX kém

**Trade-off:**
- ✅ Ưu điểm: Low latency, smooth gameplay
- ⚠️ Nhược điểm: Data có thể mất nếu server crash trước khi sync DB
- 📊 Acceptable: Game state có thể reconstruct từ logs

---

### 2. Race Condition Handling

**Problem:** 2 clients join đồng thời có thể vượt max_players

**Solution:** Re-check capacity trong [room_add_player()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Network/src/transport/room_manager.c#65-88)

**Why not mutex?**
- Single-threaded event loop (epoll) → ít race condition
- Mutex overhead không cần thiết cho scale nhỏ
- Re-check đơn giản và đủ hiệu quả

---

### 3. Buffer Management

**Issue:** ArrayBuffer reuse có thể để lại dữ liệu rác

**Fix:** `fill(0)` trước khi [set()](file:///home/duyen/DAIHOC/NetworkProgramming/Final/the-price-is-right-multiplayer/Frontend/src/services/hostService.js#136-157)

**Cost:** Negligible (8 bytes)

---

## X. GHI CHÚ TRIỂN KHAI

- **Thiết kế payload:** Format cố định 16-byte đảm bảo parsing đơn giản
- **Quy tắc visibility:** Join từ danh sách YÊU CẦU visibility công khai
- **Join bằng code:** Hoạt động cho cả phòng công khai và riêng tư
- **Tên người chơi:** Cùng pattern query DB như CREATE_ROOM
- **Broadcasts:** Hai thông báo đảm bảo tất cả clients đồng bộ
- **DB persistence:** Non-blocking, in-memory là authoritative
- **Race condition:** Protected bằng re-check trong room_add_player()
- **Buffer safety:** Clear trước khi set để tránh dữ liệu rác
