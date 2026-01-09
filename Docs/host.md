📊 PHÂN TÍCH TRIỂN KHAI HOST USE CASES
🎯 TỔNG QUAN
Dự án đã triển khai hoàn chỉnh 6 use cases cho Host và Member, sử dụng binary protocol với kiến trúc server-driven (server chủ động push notifications).

🏗️ KIẾN TRÚC TỔNG THỂ
Luồng dữ liệu 3 tầng:
Frontend (React) 
  ↓ Binary Payload Encoding (hostService.js)
  ↓ WebSocket
WS-Bridge (Node.js)
  ↓ TCP Socket
Network Server (C)
  ├── room_handler.c → Xử lý business logic
  ├── room_repo.c → Database operations (RPC calls)
  └── room_manager.c → Memory state tracking
  ↓ REST API (Supabase)
PostgreSQL Database
  ├── rooms table
  ├── room_members table
  └── RPC functions
📦 DATABASE SCHEMA
Bảng rooms:
sql
id              SERIAL PRIMARY KEY
name            VARCHAR(100)
code            VARCHAR(10) UNIQUE        -- Mã phòng 6 ký tự (VD: "ABC123")
visibility      VARCHAR(20)               -- 'public' | 'private'
host_id         INTEGER REFERENCES accounts(id)
status          VARCHAR(20)               -- 'waiting' | 'playing' | 'closed'
mode            VARCHAR(20)               -- 'scoring' | 'elimination'
max_players     INTEGER DEFAULT 5
wager_mode      BOOLEAN DEFAULT TRUE
created_at      TIMESTAMP
updated_at      TIMESTAMP
Bảng room_members:
sql
room_id         INTEGER REFERENCES rooms(id)
account_id      INTEGER REFERENCES accounts(id)
joined_at       TIMESTAMP
PRIMARY KEY (room_id, account_id)
⚠️ LƯU Ý QUAN TRỌNG:

✅ is_ready KHÔNG LƯU trong database
✅ is_ready chỉ tồn tại trong memory (room_manager.c)
✅ round_time được fix cứng trong 
protocol.h
 = 15 giây
🔧 MEMORY STATE (room_manager.c)
Cấu trúc dữ liệu:
c
typedef struct {
    int client_fd;          // Socket descriptor (volatile)
    uint32_t account_id;    // User ID từ DB (persistent)
    bool is_ready;          // ✅ CHỈ LƯU TRONG MEMORY
} RoomMember;
typedef struct {
    int room_id;
    RoomMember members[MAX_ROOM_MEMBERS];  // Tối đa 6 người
    int member_count;
} RoomState;
Các hàm chính:
room_add_member(room_id, client_fd, account_id, is_host)
Thêm member vào room
Nếu is_host = true → tự động set is_ready = true
Host luôn ready ngay khi tạo phòng
room_remove_member(room_id, client_fd)
Xóa member khỏi room
Tự động broadcast NTF_PLAYER_LEFT
Nếu room rỗng → xóa room khỏi memory
room_set_ready(room_id, client_fd, ready)
Cập nhật trạng thái ready của member
Chỉ lưu trong memory, không sync DB
room_broadcast(room_id, command, payload, len, exclude_fd)
Gửi notification đến tất cả member trong room
Có thể exclude một FD (VD: không gửi lại cho người gửi)
room_all_ready(room_id, &ready_count, &total_count)
Kiểm tra xem tất cả member đã ready chưa
Dùng để validate trước khi start game
🎮 CHI TIẾT 6 USE CASES
1️⃣ CREATE ROOM (0x0200)
Frontend (hostService.js):
javascript
// Payload: 72 bytes
// - name[64]: Tên phòng (UTF-8, null-terminated)
// - visibility: 0=public, 1=private
// - mode: 0=scoring, 1=elimination
// - max_players: 2-8
// - wager_enabled: 0=false, 1=true
// - reserved[4]: Dự phòng
const buffer = new ArrayBuffer(72);
const view = new DataView(buffer);
const nameBytes = encodeString(name, 64);
new Uint8Array(buffer, 0, 64).set(nameBytes);
view.setUint8(64, visibility === 'private' ? 1 : 0);
view.setUint8(65, mode === 'elimination' ? 1 : 0);
view.setUint8(66, maxPlayers || 6);
view.setUint8(67, wagerEnabled ? 1 : 0);
sendPacket(OPCODE.CMD_CREATE_ROOM, buffer);
Server (room_handler.c):
Validate payload size = 72 bytes
Lấy 
account_id
 từ session (không hardcode!)
Validate business rules (max_players 2-8, tên phòng không rỗng)
Gọi 
room_repo_create()
 → Tạo room trong DB
Gọi 
room_add_member(room_id, client_fd, account_id, true)
 → Track host trong memory với is_ready=true
Response RES_ROOM_CREATED cho host
Broadcast NTF_RULES_CHANGED + NTF_PLAYER_LIST cho toàn phòng
Database (room_repo.c):
Generate mã phòng 6 ký tự ngẫu nhiên (VD: "ABC123")
POST /rooms → Tạo record trong bảng rooms
POST /room_members → Thêm host vào bảng room_members
Return room_id và 
room_code
✅ Kết quả:

Room được tạo trong DB với status='waiting'
Host được thêm vào room_members
Host được track trong memory với is_ready=true
Frontend nhận được room_id, room_code để hiển thị
2️⃣ SET RULES (0x0206)
Frontend:
javascript
// Payload: 8 bytes
// - room_id (4 bytes, big-endian)
// - mode (1 byte)
// - max_players (1 byte)
// - wager_enabled (1 byte)
// - visibility (1 byte)
const buffer = new ArrayBuffer(8);
const view = new DataView(buffer);
view.setUint32(0, roomId, false); // Network byte order
view.setUint8(4, mode === 'elimination' ? 1 : 0);
view.setUint8(5, maxPlayers || 6);
view.setUint8(6, wagerMode ? 1 : 0);
view.setUint8(7, visibility === 'private' ? 1 : 0);
sendPacket(OPCODE.CMD_SET_RULE, buffer);
Server:
Validate payload = 8 bytes
Extract room_id với ntohl() (network to host byte order)
Gọi 
room_repo_set_rules()
 → Update DB qua RPC
Broadcast NTF_RULES_CHANGED cho TẤT CẢ member (bao gồm cả host)
Response RES_RULES_UPDATED cho host
Database RPC:
sql
CREATE FUNCTION update_room_rules(
    p_room_id INT,
    p_mode VARCHAR(20),
    p_max_players INT,
    p_wager_mode BOOLEAN,
    p_visibility VARCHAR(20)  -- ⭐ Đã thêm visibility
)
UPDATE rooms SET
    mode = p_mode,
    max_players = p_max_players,
    wager_mode = p_wager_mode,
    visibility = p_visibility,
    updated_at = NOW()
WHERE id = p_room_id AND status = 'waiting';
⚠️ LƯU Ý:

Trạng thái is_ready của các member KHÔNG BỊ RESET khi đổi rules
Chỉ update được khi status='waiting' (chưa bắt đầu game)
3️⃣ KICK MEMBER (0x0204)
Frontend:
javascript
// Payload: 8 bytes
// - room_id (4 bytes)
// - target_id (4 bytes) - account_id của người bị kick
const buffer = new ArrayBuffer(8);
const view = new DataView(buffer);
view.setUint32(0, roomId, false);
view.setUint32(4, targetId, false);
sendPacket(OPCODE.CMD_KICK, buffer);
Server:
Validate payload = 8 bytes
Extract room_id và target_id
Gọi 
room_repo_kick_member()
 → DELETE từ DB
Broadcast NTF_MEMBER_KICKED với {account_id: target_id}
Gọi 
room_repo_get_state()
 → Lấy danh sách member mới từ DB
Broadcast NTF_PLAYER_LIST với danh sách đã cập nhật
Response RES_MEMBER_KICKED cho host
Frontend xử lý notification:
javascript
registerHandler(OPCODE.NTF_MEMBER_KICKED, (payload) => {
    const data = JSON.parse(new TextDecoder().decode(payload));
    const myAccountId = parseInt(localStorage.getItem('account_id'));
    
    if (data.account_id === myAccountId) {
        // Tôi bị kick → redirect về lobby
        alert('You have been kicked from the room');
        window.location.href = '/lobby';
    }
    // Member khác chỉ chờ NTF_PLAYER_LIST để update UI
});
✅ Flow:

Host click "Kick" → Server xóa khỏi DB
Server broadcast NTF_MEMBER_KICKED
Người bị kick nhận được, check 
account_id
 → redirect về lobby
Server broadcast NTF_PLAYER_LIST mới
Các member còn lại update UI
4️⃣ CLOSE ROOM (0x0207)
Frontend:
javascript
// Payload: 4 bytes (room_id)
const buffer = new ArrayBuffer(4);
const view = new DataView(buffer);
view.setUint32(0, roomId, false);
sendPacket(OPCODE.CMD_CLOSE_ROOM, buffer);
Server:
Gọi 
room_repo_close()
 → RPC update status='closed' và DELETE tất cả members
Broadcast NTF_ROOM_CLOSED cho tất cả member
Response RES_ROOM_CLOSED cho host
Database RPC:
sql
CREATE FUNCTION close_room(p_room_id INT)
BEGIN
    UPDATE rooms SET status = 'closed', updated_at = NOW()
    WHERE id = p_room_id;
    
    DELETE FROM room_members WHERE room_id = p_room_id;
END;
✅ Kết quả:

Room status → 'closed'
Tất cả member bị xóa khỏi room_members
Tất cả client nhận notification và redirect về lobby
5️⃣ LEAVE ROOM (0x0202)
Frontend:
javascript
// Payload: 4 bytes (room_id)
const buffer = new ArrayBuffer(4);
const view = new DataView(buffer);
view.setUint32(0, roomId, false);
sendPacket(OPCODE.CMD_LEAVE_ROOM, buffer);
Server:
Lấy 
account_id
 từ session
Gọi 
room_repo_leave()
 → DELETE từ room_members
Broadcast NTF_PLAYER_LEFT với {account_id} (exclude người rời)
Gọi 
room_remove_member()
 → Xóa khỏi memory
Response RES_ROOM_LEFT cho client
⚠️ LƯU Ý:

Nếu host leave → room vẫn tồn tại (không tự động close)
Có thể uncomment logic trong RPC để auto-close khi host leave
6️⃣ GET ROOM STATE (0x0208)
Frontend:
javascript
// Payload: 4 bytes (room_id)
const buffer = new ArrayBuffer(4);
const view = new DataView(buffer);
view.setUint32(0, roomId, false);
sendPacket(OPCODE.CMD_GET_ROOM_STATE, buffer);
Server:
Gọi 
room_repo_get_state()
 → RPC lấy snapshot từ DB
Response RES_ROOM_STATE với JSON chứa rules + players
Database RPC (get_room_state):
sql
CREATE FUNCTION get_room_state(p_room_id INTEGER)
RETURNS json AS $$
BEGIN
    -- Lấy rules
    SELECT json_build_object(
        'mode', mode,
        'maxPlayers', max_players,
        'wagerMode', wager_mode,
        'visibility', visibility
    ) INTO v_room_data FROM rooms WHERE id = p_room_id;
    
    -- Lấy members (host luôn is_ready=true, sort host lên đầu)
    SELECT json_agg(
        json_build_object(
            'account_id', rm.account_id,
            'username', COALESCE(p.name, a.email),
            'is_host', (r.host_id = rm.account_id),
            'is_ready', (r.host_id = rm.account_id)  -- ⭐ Host auto ready
        ) ORDER BY (r.host_id = rm.account_id) DESC
    ) INTO v_members_data
    FROM room_members rm
    JOIN accounts a ON rm.account_id = a.id
    LEFT JOIN profiles p ON rm.account_id = p.account_id
    JOIN rooms r ON rm.room_id = r.id
    WHERE rm.room_id = p_room_id;
    
    RETURN json_build_object(
        'rules', v_room_data,
        'players', COALESCE(v_members_data, '[]'::json)
    );
END;
$$;
✅ Đặc điểm:

RPC function trả về is_ready=true cho host (hardcode trong SQL)
Non-host members luôn is_ready=false khi pull từ DB
Dùng để sync state khi reload page hoặc sau khi kick member
🔔 NOTIFICATION SYSTEM
Server-driven Architecture:
Server chủ động push notifications đến client khi có sự kiện:

Opcode	Tên	Khi nào gửi	Payload
700	NTF_PLAYER_JOINED	Member mới join	{account_id, username}
701	NTF_PLAYER_LEFT	Member rời phòng	{account_id}
702	NTF_PLAYER_LIST	Danh sách member thay đổi	[{account_id, username, is_host, is_ready}]
712	NTF_PLAYER_READY	Member đổi trạng thái ready	{account_id, is_ready}
713	NTF_RULES_CHANGED	Host đổi rules	{mode, max_players, wager_mode, visibility}
714	NTF_MEMBER_KICKED	Member bị kick	{account_id}
715	NTF_ROOM_CLOSED	Host đóng phòng	{success: true}
Broadcast Implementation:
c
void room_broadcast(int room_id, uint16_t command, 
                   const char *payload, uint32_t payload_len,
                   int exclude_fd) {
    RoomState *room = find_room(room_id);
    
    // Build header
    MessageHeader header;
    header.magic = htons(MAGIC_NUMBER);
    header.command = htons(command);
    header.length = htonl(payload_len);
    
    // Send to all members
    for (int i = 0; i < room->member_count; i++) {
        int fd = room->members[i].client_fd;
        if (fd != exclude_fd) {
            send(fd, &header, sizeof(header), 0);
            send(fd, payload, payload_len, 0);
        }
    }
}
⚙️ PROTOCOL CONSTANTS
protocol.h:
c
#define MAGIC_NUMBER 0x4347         // "CG" - ConsoleGame
#define PROTOCOL_VERSION 0x01
#define HEADER_SIZE 16
#define MAX_PAYLOAD_SIZE 4096
#define DEFAULT_ROUND_TIME 15       // ⭐ Fix cứng 15 giây
🎯 ĐIỂM MẠNH CỦA IMPLEMENTATION
✅ Session-based authentication - Dùng 
get_account_id(client_fd)
 thay vì hardcode
✅ Binary protocol - Struct packing chính xác, network byte order đúng
✅ State separation - DB (persistent) vs Memory (volatile) rõ ràng
✅ Broadcast system - Real-time sync cho tất cả member
✅ Error handling - Comprehensive error codes và messages
✅ Memory safety - cJSON cleanup đúng, buffer overflow protection
✅ RPC functions - Database logic được encapsulate tốt
✅ Server-driven - Client không cần poll, server chủ động push

📝 TÓM TẮT
