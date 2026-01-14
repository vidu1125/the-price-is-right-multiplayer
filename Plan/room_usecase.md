

---

# NGUYÊN TẮC CHUNG (ÁP DỤNG CHO TẤT CẢ USE CASE)

### 🔒 Connection & State Rule (bắt buộc)

* Tất cả use case dưới đây **chỉ được xử lý khi Connection.state = LOBBY hoặc IN_ROOM**
* **Không query DB để kiểm tra trạng thái**
* Server là **authoritative**
* Nếu socket:

  * UNAUTHENTICATED → trả `ERR_NOT_LOGGED_IN`
  * PLAYING → trả `ERR_BAD_REQUEST`

---

### 🧠 In-memory State Model (chuẩn để hiểu phần sau)

```c
UserSession {
  int socket_fd;
  int account_id; // null nếu chưa login
  SessionState state;
}

enum SessionState {
  SESSION_UNAUTHENTICATED,
  SESSION_LOBBY,
  SESSION_PLAYING,
  SESSION_PLAYING_DISCONNECTED
};

RoomState {
  uint32_t id;              // maps to rooms.id
  char     name[32];        // maps to rooms.name
  char     code[8];         // maps to rooms.code

  uint32_t host_id;         // maps to rooms.host_id

  enum RoomStatus {
    ROOM_WAITING,           // maps to rooms.status = 'waiting'
    ROOM_PLAYING,           // maps to rooms.status = 'playing'
    ROOM_CLOSED             // maps to rooms.status = 'closed'
  } status;

  enum GameMode {
    MODE_ELIMINATION,       // maps to rooms.mode = 'elimination'
    MODE_SCORING            // maps to rooms.mode = 'scoring'
  } mode;

  uint8_t max_players;      // maps to rooms.max_players

  enum RoomVisibility {
    ROOM_PUBLIC,            // maps to rooms.visibility = 'public'
    ROOM_PRIVATE            // maps to rooms.visibility = 'private'
  } visibility;

  bool wager_mode;          // maps to rooms.wager_mode

  Map<account_id, RoomPlayerState> players; // ❌ không có trong DB
}




RoomPlayerState {
  uint32_t account_id;
  char name[64];         // Player's display name (fetched from DB on join, cached in memory)
  char avatar[256];      // Player's avatar URL (fetched from DB on join, cached in memory)

  bool is_host;      // true nếu account_id == RoomState.host_id
  bool is_ready;     // REAL-TIME: player đã sẵn sàng chưa
  bool connected;    // REAL-TIME: socket còn sống không
  time_t joined_at;  // REAL-TIME: thời điểm join (dùng cho host transfer)
}

```

### B. Bảng mapping chính thức (CỰC KỲ QUAN TRỌNG CHO REPORT)

| In-memory (RoomState) | Database (rooms)   |
| --------------------- | ------------------ |
| id                    | rooms.id           |
| name                  | rooms.name         |
| code                  | rooms.code         |
| host_id               | rooms.host_id      |
| status                | rooms.status       |
| mode                  | rooms.mode         |
| max_players           | rooms.max_players  |
| visibility            | rooms.visibility   |
| wager_mode            | rooms.wager_mode   |
| players               | ❌ (in-memory only) |

2️⃣ QUY TẮC SUY LUẬN “USER Ở TRONG ROOM”

Một user đang ở trong room khi:

UserSession.state == SESSION_LOBBY

AND tồn tại RoomPlayerState của user trong RoomState.players

👉 Không có state riêng cho IN_ROOM

3️⃣ QUY TẮC XỬ LÝ USE CASE LOBBY

Chỉ xử lý khi:

UserSession.state == SESSION_LOBBY

Nếu:

SESSION_UNAUTHENTICATED → ERR_NOT_LOGGED_IN

SESSION_PLAYING / SESSION_PLAYING_DISCONNECTED → ERR_BAD_REQUEST

KHÔNG query DB để check realtime

---

### C. Định nghĩa Payload cho Notifications

#### NTF_PLAYER_LIST (0x02BE)

Thông báo danh sách player hiện tại trong phòng (broadcast sau mọi thay đổi membership).

**Implementation:** JSON payload (UTF-8 encoded)

```json
{
  "members": [
    {
      "account_id": 42,
      "name": "PlayerName",
      "avatar": "",
      "is_host": true,
      "is_ready": false
    }
  ]
}
```

**Field descriptions:**
- `account_id` (number): ID của player
- `name` (string): Tên hiển thị của player (lấy từ `profiles.name` khi join, cached trong `RoomPlayerState.name`)
- `avatar` (string): Avatar URL của player (lấy từ `profiles.avatar` khi join, cached trong `RoomPlayerState.avatar`)
- `is_host` (boolean): Player có phải host không
- `is_ready` (boolean): Player đã sẵn sàng chưa

**Data flow:**
1. Khi player join room, server query `profiles` table để lấy `name` và `avatar`
2. `name` và `avatar` được lưu vào `RoomPlayerState` (in-memory cache)
3. Khi broadcast `NTF_PLAYER_LIST`, server đọc từ `RoomPlayerState` (không query DB lại)

**Max payload:** ~500 bytes cho 6 players (an toàn trong giới hạn 4096 bytes)

📌 **Lý do dùng JSON thay vì binary:**
- Dễ debug và maintain
- Frontend dễ parse
- Flexible cho future fields
- Payload size vẫn nhỏ (< 1KB cho 6 players)

---
# ==============================
# USE CASE 1️⃣ – CREATE NEW ROOM
# ==============================

## 🎯 Goal

User tạo phòng mới với **đầy đủ cấu hình** và trở thành **host** của phòng.

---

## ✅ Pre-condition

- `UserSession.state == SESSION_LOBBY`
- User **chưa tồn tại trong bất kỳ RoomState nào**

---

## 🌐 Network Layer

### Client → Server

```

CMD_CREATE_ROOM (0x0200)

````

### Payload

```c
#define ROOM_NAME_MAX 32

typedef struct PACKED {
  char     name[ROOM_NAME_MAX]; // Tên phòng (UTF-8, null-terminated)
  uint8_t  mode;                // MODE_ELIMINATION | MODE_SCORING
  uint8_t  max_players;         // ELIM=4, SCORING=4..6
  uint8_t  visibility;          // ROOM_PUBLIC | ROOM_PRIVATE
  uint8_t  wager_mode;          // 0 = OFF, 1 = ON
} CreateRoomPayload;
````

---

## 📏 Payload Size

| Field       | Size         |
| ----------- | ------------ |
| name[32]    | 32           |
| mode        | 1            |
| max_players | 1            |
| visibility  | 1            |
| wager_mode  | 1            |
| **Total**   | **36 bytes** |

* Payload tối đa cho phép: **4096 bytes**
* Payload thực tế: **36 bytes**

➡️ An toàn tuyệt đối, không chạm `ERR_PAYLOAD_LARGE (405)`.

---

## 🧠 Application Layer (Server xử lý)

### 1️⃣ Validate Session & State

* Nếu `SESSION_UNAUTHENTICATED` → `ERR_NOT_LOGGED_IN (401)`
* Nếu user đã tồn tại trong một `RoomState` → `ERR_BAD_REQUEST (400)`

---

### 2️⃣ Validate Payload

* `name`

  * không rỗng
  * độ dài: 1-32 bytes (UTF-8)
  * UTF-8 hợp lệ
  * không chứa ký tự đặc biệt nguy hiểm: `<>"'&;`
* `mode`

  * chỉ nhận `MODE_ELIMINATION` hoặc `MODE_SCORING`
* `max_players`

  * ELIMINATION → **bắt buộc = 4**
  * SCORING → **4 ≤ max_players ≤ 6**
* `visibility`

  * PUBLIC hoặc PRIVATE
* `wager_mode`

  * 0 hoặc 1

📌 **Server là authoritative**: client gửi cấu hình, server quyết định hợp lệ hay không.

---

### 3️⃣ Tạo RoomState (IN-MEMORY)

```c
RoomState room;

room.id          = generate_id();          // maps to rooms.id
room.name        = payload.name;            // maps to rooms.name
room.code        = generate_code();         // maps to rooms.code
room.host_id     = account_id;              // maps to rooms.host_id

room.status      = ROOM_WAITING;            // maps to rooms.status = 'waiting'
room.mode        = payload.mode;            // maps to rooms.mode
room.max_players = payload.max_players;     // maps to rooms.max_players
room.visibility  = payload.visibility;      // maps to rooms.visibility
room.wager_mode  = payload.wager_mode;      // maps to rooms.wager_mode
```

### Add Host vào RoomState

**Implementation:** Sử dụng `room_add_player()` với player name

```c
// STEP 1: Fetch host's profile name from DB
char profile_name[64] = "Host";  // default fallback
char query[128];
snprintf(query, sizeof(query), "account_id=eq.%u", account_id);

cJSON *profile_response = NULL;
if (db_get("profiles", query, &profile_response) == DB_OK && profile_response) {
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

// STEP 2: Add host to room using room_add_player()
// Signature: int room_add_player(uint32_t room_id, uint32_t account_id, const char *name, int client_fd)
room_add_player(room->id, account_id, profile_name, client_fd);

// STEP 3: Mark as host
room->players[0].is_host = true;
```

**What happens inside `room_add_player()`:**
```c
RoomPlayerState *player = &room->players[room->player_count];
player->account_id = account_id;
strncpy(player->name, name ? name : "Player", sizeof(player->name) - 1);
player->name[sizeof(player->name) - 1] = '\0';
player->is_host = false;      // Will be set to true after for host
player->is_ready = false;     // REAL-TIME ONLY
player->connected = true;     // REAL-TIME ONLY
player->joined_at = time(NULL);
```

📌 **Player name caching strategy:**
- Query `profiles.name` **once** when player joins
- Store in `RoomPlayerState.name` (in-memory cache)
- Use cached name for all broadcasts (no repeated DB queries)
- Name is snapshot at join time (doesn't change if profile updates)

---

### 4️⃣ Persist Database (NON-REALTIME)

Insert vào bảng `rooms`:

* `id`
* `name`
* `code`
* `host_id`
* `status`
* `mode`
* `max_players`
* `visibility`
* `wager_mode`
* `created_at`

Insert vào bảng `room_members`:

* `room_id`
* `account_id` (host)

📌 **Không lưu**:

* `is_ready`
* `connected`
* `players` (runtime only)

---

## 📤 Server → Client

### Response

```
RES_ROOM_CREATED (0x00DC)
```

Payload:

```c
#define ROOM_CODE_LEN 8

typedef struct PACKED {
    uint32_t room_id;                  // network byte order
    char     room_code[ROOM_CODE_LEN]; // null-terminated nếu < 8
} CreateRoomResponsePayload;
```

### Notification (cho chính host)

```
NTF_PLAYER_LIST (0x02BE)
```

Payload: danh sách player hiện tại trong phòng (chỉ host).

---

## ❌ Error Cases

| Điều kiện         | Response                |
| ----------------- | ----------------------- |
| Chưa login        | ERR_NOT_LOGGED_IN (401) |
| Đã ở room         | ERR_BAD_REQUEST (400)   |
| Rule không hợp lệ | ERR_BAD_REQUEST (400)   |
| Payload sai       | ERR_BAD_REQUEST (400)   |

---
# ==============================
# USE CASE 2️⃣ – JOIN ROOM
# ==============================

## 🎯 Goal

User tham gia một phòng chờ hợp lệ (public list hoặc private code).

---

## ✅ Pre-condition

- `UserSession.state == SESSION_LOBBY`
- User **chưa tồn tại trong bất kỳ RoomState nào**

---

## 🌐 Network Layer

### Client → Server

```

CMD_JOIN_ROOM (0x0201)

````

### Payload

**Fixed-length design** - Server không cần quan tâm join type ở logic game.

```c
typedef struct PACKED {
  uint8_t  by_code;      // 0 = join by room_id, 1 = join by room_code
  uint8_t  reserved[3];  // padding for alignment
  uint32_t room_id;      // network byte order - dùng nếu by_code = 0
  char     room_code[8]; // null-terminated - dùng nếu by_code = 1
} JoinRoomPayload;
```

**Total size:** 16 bytes (fixed)

📌 **Ưu điểm:**
- Payload luôn cố định 16 bytes → parsing đơn giản
- Server chỉ check 1 byte `by_code` để biết đọc field nào
- Logic game không quan tâm join type
- Alignment tốt (4-byte boundaries)

---

## 🧠 Application Layer (Server xử lý)

1. Validate `SESSION_LOBBY`
2. Resolve `RoomState`:

   * ưu tiên in-memory
   * fallback DB nếu mới khởi động server
3. Validate:

   * `RoomState.status == ROOM_WAITING`
   * Capacity:

     * ELIMINATION → `players.size() < 4` (cho phép join khi có 0-3 người)
     * SCORING → `players.size() < max_players`
4. Nếu join bằng **room_id (list)**:

   * `RoomState.visibility == ROOM_PUBLIC`
5. **Fetch player name from DB:**

   ```c
   char profile_name[64] = "Player";
   char query[128];
   snprintf(query, sizeof(query), "account_id=eq.%u", account_id);
   
   cJSON *profile_response = NULL;
   if (db_get("profiles", query, &profile_response) == DB_OK && profile_response) {
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
   ```

6. Add `RoomPlayerState` using `room_add_player()`:

   ```c
   // Signature: int room_add_player(uint32_t room_id, uint32_t account_id, 
   //                                 const char *name, const char *avatar, int client_fd)
   room_add_player(room_id, account_id, profile_name, profile_avatar, client_fd);
   // Player is automatically added with:
   // - is_host = false
   // - is_ready = false
   // - connected = true
   // - name = profile_name (cached)
   // - avatar = profile_avatar (cached)
   ```
7. Insert DB:

   * `room_members(room_id, account_id)`

---

## 📤 Server → Client

### Response

```
RES_ROOM_JOINED (0x00DD)
```

**Payload:** JSON object (UTF-8 encoded) containing full room state

```json
{
  "roomId": 123,
  "roomCode": "ABC123",
  "roomName": "My Room",
  "hostId": 47,
  "isHost": false,
  "gameRules": {
    "mode": "elimination",
    "maxPlayers": 4,
    "wagerMode": true,
    "visibility": "public"
  },
  "players": [
    {
      "account_id": 47,
      "name": "Host Player",
      "avatar": "",
      "is_host": true,
      "is_ready": false
    },
    {
      "account_id": 42,
      "name": "Joining Player",
      "avatar": "",
      "is_host": false,
      "is_ready": false
    }
  ]
}
```

**Design rationale:**
- Prevents race condition where `NTF_PLAYER_LIST` arrives before `WaitingRoom` mounts
- Joiner receives complete room state immediately
- Enables instant UI rendering without waiting for broadcasts

### Notifications (broadcast)

```
NTF_PLAYER_JOINED (0x02BC)
NTF_PLAYER_LIST   (0x02BE)
```

---

## ❌ Error Cases

| Điều kiện                | Response               |
| ------------------------ | ---------------------- |
| Phòng đầy                | ERR_ROOM_FULL (403)    |
| Game đã start            | ERR_GAME_STARTED (404) |
| Private room (join list) | ERR_BAD_REQUEST (400)  |
| Room không tồn tại       | ERR_BAD_REQUEST (400)  |

---

# ==============================

# USE CASE 3️⃣ – SET GAME RULE 

# ==============================

## 🎯 Goal

Host **commit** cấu hình phòng mới trước khi game bắt đầu.
Luật chơi **chỉ thay đổi khi host xác nhận (Done)**, không thay đổi theo từng thao tác chỉnh UI.

---

## ✅ Pre-condition

* `UserSession.state == SESSION_LOBBY`
* User là host (`account_id == RoomState.host_id`)
* `RoomState.status == ROOM_WAITING`

---

## 🌐 Network Layer

### Client → Server

```
CMD_SET_RULE (0x0206)
```

> Lệnh này **chỉ được gửi khi host bấm “Done”** sau khi chỉnh rule trên UI.

### Payload

```c
typedef struct PACKED {
  uint8_t mode;          // MODE_ELIMINATION | MODE_SCORING
  uint8_t max_players;   // ELIM=4, SCORING=4..6
  uint8_t visibility;    // ROOM_PUBLIC | ROOM_PRIVATE
  uint8_t wager_mode;    // 0 | 1
} SetRulePayload;
```

Payload đại diện cho **toàn bộ cấu hình cuối cùng** mà host muốn áp dụng.

---

## 🧠 Application Layer

1. Validate:

   * user là host
   * room đang ở trạng thái `ROOM_WAITING`

2. Validate rule:

   * ELIMINATION → `max_players == 4`
   * SCORING → `4 ≤ max_players ≤ 6`

3. **Commit rule**:

   * Update `RoomState` (in-memory):

     * `mode`
     * `max_players`
     * `visibility`
     * `wager_mode`
   * (Optional) Update DB `rooms` để đồng bộ cấu hình

4. Reset `is_ready` của **toàn bộ `RoomPlayerState`**

   > Vì rule thay đổi → trạng thái sẵn sàng trước đó không còn hợp lệ

---

## 📤 Server → Client

### Response (unicast – chỉ gửi cho host)

```
RES_RULES_UPDATED (0x00E0)
```

Xác nhận server đã **chấp nhận và commit** cấu hình mới.

---

### Notification (broadcast – gửi cho toàn bộ player trong phòng)

```
NTF_RULES_CHANGED (0x02C9)
NTF_PLAYER_LIST  (0x02BE)
```

* `NTF_RULES_CHANGED`: thông báo rule đã thay đổi, client cập nhật UI
* `NTF_PLAYER_LIST`: cập nhật lại trạng thái ready (tất cả = false)

---

## ❌ Error Cases

| Điều kiện            | Response              |
| -------------------- | --------------------- |
| Không phải host      | ERR_NOT_HOST (406)    |
| Room không ở WAITING | ERR_BAD_REQUEST (400) |
| Rule sai             | ERR_BAD_REQUEST (400) |

---

### 🔒 Ghi chú thiết kế (ngầm hiểu)

* Không tồn tại trạng thái “draft rule” trên server
* Nếu host **chưa bấm Done** → **không gửi CMD_SET_RULE**, rule cũ vẫn giữ nguyên
* Mọi thay đổi rule đều là **atomic & authoritative từ server**


# ==============================

# USE CASE 4️⃣ – READY (FINAL)

# ==============================

## 🎯 Goal

Player báo đã sẵn sàng.

---

## ✅ Pre-condition

* `UserSession.state == SESSION_LOBBY`
* User tồn tại trong `RoomState.players`

---

## 🌐 Network Layer

### Client → Server

```
CMD_READY (0x0203)
Payload: empty
```

---

## 🧠 Application Layer

1. Set:

   ```c
   player->is_ready = true;
   ```
2. Không thay đổi DB (real-time only)

---

## 📤 Server → Client

### Notification

```
NTF_PLAYER_READY (0x02C8)
```

---

## ❌ Error Cases

| Điều kiện          | Response              |
| ------------------ | --------------------- |
| Không ở trong room | ERR_BAD_REQUEST (400) |

---

# ==============================

# USE CASE 5️⃣ – LEAVE ROOM 

# ==============================

## 🎯 Goal

Player rời phòng chờ.
Server phải:

* Broadcast cho các player còn lại biết player đã rời phòng
* Nếu player rời là host → **chuyển quyền host** cho người vào sớm nhất còn lại

---

## ✅ Pre-condition

* `UserSession.state == SESSION_LOBBY`
* User tồn tại trong `RoomState.players`
* `RoomState.status == ROOM_WAITING`
  (Nếu `ROOM_PLAYING` thì use case rời phòng phải đi theo `CMD_FORFEIT`, không dùng `CMD_LEAVE_ROOM`)

---

## 🌐 Network Layer

### Client → Server

```
CMD_LEAVE_ROOM (0x0202)
Payload: empty
```

---

## 🧠 Application Layer
### 1) Validate (REAL-TIME – IN-MEMORY)

1. Check user đang ở trong room
   → tồn tại trong `RoomState.players`
2. Check `RoomState.status == ROOM_WAITING`

   * nếu không → trả `ERR_BAD_REQUEST (400)`

📌 **Không query DB** ở bước này.
📌 Mọi kiểm tra realtime **chỉ dựa trên in-memory state**.

---

### 2) Remove player (REAL-TIME + PERSISTENCE)

#### a) In-memory (authoritative)

* Remove `RoomPlayerState` tương ứng khỏi room:

```c
room.players.erase(account_id);
```

Sau bước này:

* user **không còn thuộc phòng**
* server **không broadcast lobby notification** cho user này nữa

#### b) Database (persistence, non-realtime)

* Xóa membership tương ứng trong DB:

```sql
DELETE FROM room_members
WHERE room_id = :room_id
  AND account_id = :account_id;
```

📌 Bảng `room_members` **chỉ lưu membership hiện tại**.
📌 Không lưu lý do rời phòng (leave / kick).
📌 DB phải luôn **nhất quán với in-memory state**, dù không dùng để check realtime.

---

### 3) Host Transfer (IN-MEMORY, DB OPTIONAL)

Nếu `leaver.account_id == RoomState.host_id`:

#### Case A — Phòng còn người

1. Chọn `new_host_id` = người **vào phòng sớm nhất** trong số còn lại

   * dựa trên `join_order` / `joined_at` (**in-memory**)

2. Update in-memory:

```c
RoomState.host_id = new_host_id;
RoomState.players[new_host_id].is_host = true;
```

3. Với các player khác:

```c
is_host = false;
```

📌 **Host transfer là realtime decision → chỉ xử lý in-memory**.
📌 DB **không bắt buộc** phải update `host_id` ngay (tuỳ chiến lược persistence).

---

#### Case B — Phòng trống

* Nếu sau khi remove không còn player:

**In-memory**

```c
Destroy RoomState;   // cleanup
```

**Database**

```sql
UPDATE rooms
SET status = 'closed'
WHERE id = :room_id;
```

📌 DB phản ánh **lifecycle cuối cùng của room**.

---

### 4) Ready State Handling (REAL-TIME)

* Khi một player rời phòng, trạng thái `is_ready` của các player còn lại **được giữ nguyên**.

* Lý do:

  * `is_ready` là trạng thái realtime thuộc về từng player
  * Việc rời phòng **không làm thay đổi luật chơi**
  * Server không tự ý thay đổi quyết định ready của player khác

📌 Không update DB (ready là runtime-only).

---

## 📤 Server → Client

### Response (unicast – chỉ gửi cho người rời)

```
RES_ROOM_LEFT (0x00DE)
```

Payload: empty

> Nếu client leave chủ động, response này giúp UI đóng WaitingRoom và quay về Lobby.

---

### Notification (broadcast – gửi cho tất cả player còn lại trong phòng)

**Bắt buộc gửi:**

```
NTF_PLAYER_LEFT (0x02BD)
NTF_PLAYER_LIST (0x02BE)
```

* `NTF_PLAYER_LEFT`: thông báo ai vừa rời (để hiện toast/log)
* `NTF_PLAYER_LIST`: cập nhật danh sách player + ready + host mới (nếu có)

**Lưu ý quan trọng:**

* Không cần tạo opcode mới cho “host changed” nếu `NTF_PLAYER_LIST` đã chứa trường `is_host` đúng.
* Nếu UI cần toast “Host changed”, client suy ra bằng cách:

  * so sánh host_id/is_host trước và sau khi nhận list.

---

## ❌ Error Cases

| Điều kiện         | Response              |
| ----------------- | --------------------- |
| Không ở room      | ERR_BAD_REQUEST (400) |
| Room đang PLAYING | ERR_BAD_REQUEST (400) |

---
---

# ==============================

# USE CASE 6️⃣ – KICK MEMBER (FINAL – WITH BROADCAST & STATE)

# ==============================

## 🎯 Goal

Host loại một player khỏi phòng chờ.
Server phải:

* Loại player khỏi RoomState
* Thông báo cho **toàn bộ player còn lại trong phòng**
* Cập nhật đúng state của người bị kick

---

## ✅ Pre-condition

* `UserSession.state == SESSION_LOBBY`
* User là host (`account_id == RoomState.host_id`)
* `RoomState.status == ROOM_WAITING`
* Target tồn tại trong `RoomState.players`

---

## 🌐 Network Layer

### Client → Server

```
CMD_KICK (0x0204)
```

### Payload

```c
typedef struct PACKED {
  uint32_t target_account_id; // network byte order
} KickMemberPayload;
```

---

## 🧠 Application Layer

### 1️⃣ Validate

1. Check `SESSION_LOBBY`
2. Check user là host
3. Check target tồn tại trong `RoomState.players`
4. Check `RoomState.status == ROOM_WAITING`

---

### 2️⃣ Remove target (in-memory)

* Remove `RoomPlayerState` của target khỏi `RoomState.players`
```c
room.players.erase(target_account_id);
```
* Cập nhật session của target:

```c
target_session.state = SESSION_LOBBY;
```

📌 Sau bước này, target **không còn thuộc bất kỳ RoomState nào** trong hệ thống.

---

### 3️⃣ Persist Database (NON-REALTIME)

* Xóa membership tương ứng trong DB:
```sql
DELETE FROM room_members
WHERE room_id = :room_id
  AND account_id = :target_account_id;
```

📌 Database chỉ lưu membership hiện tại, không lưu thông tin “bị kick” hay “leave”.
📌 Mọi quyết định realtime không dựa vào DB, nhưng DB phải luôn nhất quán với in-memory state.

---

### 4️⃣ Ready State Handling

Khi một player bị kick khỏi phòng, trạng thái is_ready của các player còn lại được giữ nguyên.

---

## 📤 Server → Client

### Response (unicast – gửi cho host)

```
RES_MEMBER_KICKED (0x00E1)
```

Payload: empty
→ xác nhận host kick thành công.

---

### Notification (unicast – gửi cho người bị kick)

```
NTF_MEMBER_KICKED (0x02CA)
```

#### Payload

```c
typedef struct PACKED {
  uint32_t room_id;    // room mà player bị kick khỏi (network byte order)
} MemberKickedPayload;
```

→ Client bị kick:

* đóng WaitingRoom
* quay về Lobby
* hiển thị thông báo “You were kicked from the room”

---

### Notification (broadcast – gửi cho tất cả player còn lại trong phòng)

```
NTF_PLAYER_LEFT (0x02BD)
NTF_PLAYER_LIST (0x02BE)
```

* `NTF_PLAYER_LEFT`:

  * thông báo **player X đã bị kick**
  * dùng cho toast/log UI

Payload:

```c
typedef struct PACKED {
  uint32_t account_id; // target_account_id (network byte order)
} PlayerLeftPayload;
```

* `NTF_PLAYER_LIST`:

  * snapshot trạng thái lobby mới nhất
  * cập nhật danh sách player + ready + host

📌 **Không cần opcode riêng cho “kicked” với room còn lại**
→ UI chỉ cần hiển thị lý do khác nhau (leave vs kick) nếu muốn.

---

## ❌ Error Cases

| Điều kiện            | Response              |
| -------------------- | --------------------- |
| Không phải host      | ERR_NOT_HOST (406)    |
| Target không tồn tại | ERR_BAD_REQUEST (400) |
| Room không WAITING   | ERR_BAD_REQUEST (400) |

---

## 🔑 Important State Rules (Plan Insert)

* Player bị kick:

  * bị remove khỏi `RoomState.players`
  * `UserSession.state` quay về `SESSION_LOBBY`
  * không còn quyền nhận bất kỳ lobby/game notification nào của room cũ

* Room còn lại:

  * luôn cập nhật state thông qua `NTF_PLAYER_LIST`
  * không suy luận state từ `NTF_MEMBER_KICKED`
