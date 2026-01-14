# Test Plan: JOIN_ROOM Use Case

## I. TỔNG QUAN

### Mục Đích
Tài liệu này mô tả chi tiết kế hoạch kiểm thử cho chức năng JOIN_ROOM, bao gồm các test case, kịch bản kiểm thử, và tiêu chí đánh giá.

### Phạm Vi
- **Backend**: Xử lý `CMD_JOIN_ROOM`, validation, database persistence
- **Frontend**: UI join room, navigation, state management
- **Integration**: End-to-end flow từ UI đến database

### Môi Trường Test
- **Backend**: Docker container `tpir-network`
- **Frontend**: React development server (port 3000)
- **Database**: Supabase PostgreSQL
- **WebSocket**: Gateway bridge (port 5500)

---

## II. FUNCTIONAL TEST CASES

### TC-JOIN-001: Join Public Room by ID (Happy Path)

**Preconditions:**
- User A đã tạo phòng công khai (ID: 123, Code: ABC123)
- User B đã đăng nhập, đang ở lobby
- Phòng chưa đầy (players < max_players)

**Steps:**
1. User B click vào phòng trong danh sách public rooms
2. Click nút "Join"
3. Quan sát UI transition

**Expected Results:**
- ✅ Server nhận `CMD_JOIN_ROOM` với `by_code=0`, `room_id=123`
- ✅ Server validate session state = `SESSION_LOBBY`
- ✅ Server kiểm tra User B chưa ở trong phòng nào
- ✅ Server tìm thấy room 123 trong memory
- ✅ Server validate room status = `ROOM_WAITING`
- ✅ Server validate capacity (players < max_players)
- ✅ Server query `profiles` để lấy name và avatar của User B
- ✅ Server gọi `room_add_player()` thành công
- ✅ Server insert vào `room_members` table
- ✅ Server gửi `RES_ROOM_JOINED` với JSON payload chứa:
  - `roomId`, `roomCode`, `roomName`, `hostId`
  - `gameRules` object
  - `players` array (bao gồm User A và User B)
- ✅ Server broadcast `NTF_PLAYER_JOINED` cho User A
- ✅ Server broadcast `NTF_PLAYER_LIST` cho cả hai users
- ✅ Frontend User B navigate đến `/waitingroom`
- ✅ `WaitingRoom` component hiển thị:
  - Tên phòng: "My Room"
  - Game rules chính xác
  - Danh sách 2 players (User A - host, User B - member)
  - Avatar của cả hai (hoặc default nếu null)
- ✅ User A thấy User B xuất hiện trong member list
- ✅ Database `room_members` có record mới cho User B

**Logs to Verify:**
```
[SERVER] [JOIN_ROOM] Request from fd=6
[SERVER] [JOIN_ROOM] by_code=0, room_id=123
[SERVER] [JOIN_ROOM] Found room 123 (ABC123), status=0, players=1/4
[SERVER] [JOIN_ROOM] Player name: UserBName
[SERVER] Added player: id=42, name='UserBName' to room 123
[SERVER] [JOIN_ROOM] ✅ THÀNH CÔNG: người chơi 42 join phòng 123
[ROOM] Broadcasting player list for room 123: {"members":[...]}
```

---

### TC-JOIN-002: Join Private Room by Code

**Preconditions:**
- User A đã tạo phòng riêng tư (Code: XYZ789)
- User B đã đăng nhập, có mã phòng

**Steps:**
1. User B click "Join by Code"
2. Nhập mã "XYZ789"
3. Click "Join"

**Expected Results:**
- ✅ Frontend gửi `CMD_JOIN_ROOM` với `by_code=1`, `room_code="XYZ789"`
- ✅ Server tìm phòng bằng `find_room_by_code("XYZ789")`
- ✅ Server validate phòng tồn tại
- ✅ Server cho phép join (private room via code is allowed)
- ✅ Join flow giống TC-JOIN-001
- ✅ User B navigate đến waiting room thành công

---

### TC-JOIN-003: Reject - Room Full (Elimination Mode)

**Preconditions:**
- Phòng elimination đã có 4 players
- User E cố gắng join

**Steps:**
1. User E click "Join" trên phòng đầy

**Expected Results:**
- ✅ Server validate `room->player_count >= 4`
- ✅ Server gửi `ERR_ROOM_FULL (0x0193)`
- ✅ Frontend hiển thị alert: "Phòng đã đầy"
- ✅ User E vẫn ở lobby
- ✅ Không có record mới trong `room_members`

**Logs to Verify:**
```
[SERVER] [JOIN_ROOM] Error: Room full (elimination mode, 4/4)
```

---

### TC-JOIN-004: Reject - Room Full (Scoring Mode)

**Preconditions:**
- Phòng scoring với max_players=6 đã có 6 players
- User G cố gắng join

**Steps:**
1. User G click "Join"

**Expected Results:**
- ✅ Server validate `room->player_count >= room->max_players`
- ✅ Server gửi `ERR_ROOM_FULL`
- ✅ Frontend hiển thị lỗi
- ✅ User G không được thêm vào phòng

---

### TC-JOIN-005: Reject - Game Already Started

**Preconditions:**
- Phòng đã chuyển sang `ROOM_PLAYING`

**Steps:**
1. User X cố gắng join phòng đang chơi

**Expected Results:**
- ✅ Server validate `room->status != ROOM_WAITING`
- ✅ Server gửi `ERR_GAME_STARTED (0x0194)`
- ✅ Frontend hiển thị: "Game đã bắt đầu"

**Logs to Verify:**
```
[SERVER] [JOIN_ROOM] Error: Game already started (status=1)
```

---

### TC-JOIN-006: Reject - User Already in Room

**Preconditions:**
- User C đã ở trong phòng 123

**Steps:**
1. User C cố gắng join phòng 456

**Expected Results:**
- ✅ Server gọi `room_user_in_any_room(account_id)`
- ✅ Phát hiện User C đã ở phòng 123
- ✅ Server gửi `ERR_BAD_REQUEST` với message "Already in a room"
- ✅ Frontend hiển thị lỗi
- ✅ User C vẫn ở phòng 123

**Logs to Verify:**
```
[SERVER] [JOIN_ROOM] Error: User 42 already in a room
```

---

### TC-JOIN-007: Reject - Private Room via Public List

**Preconditions:**
- Phòng 789 có `visibility=ROOM_PRIVATE`
- User D cố gắng join từ public list

**Steps:**
1. User D click "Join" từ danh sách (nếu hiển thị do bug)

**Expected Results:**
- ✅ Server validate `room->visibility == ROOM_PUBLIC` khi `by_code=0`
- ✅ Server gửi `ERR_BAD_REQUEST` với message "Room is private"
- ✅ Frontend hiển thị lỗi

**Note:** Frontend nên ẩn private rooms khỏi public list (UI prevention)

---

### TC-JOIN-008: Reject - Invalid Room Code

**Preconditions:**
- Không có phòng nào với code "FAKE99"

**Steps:**
1. User F nhập code "FAKE99"
2. Click "Join"

**Expected Results:**
- ✅ Server gọi `find_room_by_code("FAKE99")`
- ✅ Trả về NULL
- ✅ Server gửi `ERR_BAD_REQUEST` với message "Invalid room code"
- ✅ Frontend hiển thị: "Mã phòng không hợp lệ"

**Logs to Verify:**
```
[SERVER] [JOIN_ROOM] Error: Invalid room code 'FAKE99'
```

---

### TC-JOIN-009: Reject - Not Logged In

**Preconditions:**
- User session state = `SESSION_UNAUTHENTICATED`

**Steps:**
1. Unauthenticated user gửi `CMD_JOIN_ROOM`

**Expected Results:**
- ✅ Server validate session
- ✅ Server gửi `ERR_NOT_LOGGED_IN (0x0191)`
- ✅ Frontend redirect đến login page

---

### TC-JOIN-010: Reject - Invalid State (Playing)

**Preconditions:**
- User session state = `SESSION_PLAYING`

**Steps:**
1. User đang chơi game cố gắng join phòng khác

**Expected Results:**
- ✅ Server validate `session->state == SESSION_LOBBY`
- ✅ Server gửi `ERR_BAD_REQUEST` với message "Invalid state"

---

## III. RACE CONDITION TESTS

### TC-RACE-001: Multiple Concurrent Joins

**Scenario:** 2 users join cùng lúc vào phòng còn 1 slot

**Preconditions:**
- Phòng có max_players=4, hiện có 3 players

**Steps:**
1. User D và User E click "Join" đồng thời (< 100ms apart)
2. Cả hai request đến server gần như cùng lúc

**Expected Results:**
- ✅ Request đầu tiên pass capacity check
- ✅ `room_add_player()` re-check capacity
- ✅ Player đầu tiên được thêm vào (player_count = 4)
- ✅ Request thứ hai fail tại `room_add_player()` hoặc handler capacity check
- ✅ Chỉ 1 user nhận `RES_ROOM_JOINED`
- ✅ User kia nhận `ERR_ROOM_FULL`
- ✅ Database chỉ có 4 members

**Logs to Verify:**
```
[ROOM] Added player: id=42, name='UserD' to room 123
[SERVER] [JOIN_ROOM] Error: Room full (4/4)
```

---

### TC-RACE-002: Player List Broadcast Timing

**Scenario:** Verify joiner receives player list immediately

**Preconditions:**
- User A đã ở trong phòng
- User B join

**Steps:**
1. User B join phòng
2. Monitor network packets và React component lifecycle

**Expected Results:**
- ✅ `RES_ROOM_JOINED` chứa `players` array với cả User A và User B
- ✅ Frontend `RoomPanel.js` pass `players` qua navigation state
- ✅ `WaitingRoom.js` khởi tạo `room.players` từ `location.state.players`
- ✅ `MemberListPanel` render ngay lập tức với 2 players
- ✅ `NTF_PLAYER_LIST` đến sau (hoặc bị miss) không ảnh hưởng UI
- ✅ Không có khoảng thời gian nào UI hiển thị 0 players

**Frontend Logs to Verify:**
```
[WaitingRoom] location.state: {roomId: 123, players: [{...}, {...}]}
[WaitingRoom] Initial room state: {players: [{...}, {...}]}
```

---

## IV. DATA INTEGRITY TESTS

### TC-DATA-001: Player Name Caching

**Scenario:** Verify player name được cache từ DB

**Steps:**
1. User join phòng
2. Kiểm tra server logs
3. User update profile name trong DB
4. Kiểm tra waiting room UI

**Expected Results:**
- ✅ Server query `profiles` table 1 lần khi join
- ✅ `RoomPlayerState.name` được set từ `profiles.name`
- ✅ `NTF_PLAYER_LIST` sử dụng cached name
- ✅ Nếu user update profile, waiting room vẫn hiển thị tên cũ (snapshot)
- ✅ Tên mới chỉ áp dụng cho lần join tiếp theo

**Logs to Verify:**
```
[DB_GET] Request URL: .../profiles?account_id=eq.42
[SERVER] [JOIN_ROOM] Player name: OldName
[ROOM] Added player: id=42, name='OldName' to room 123
```

---

### TC-DATA-002: Avatar Handling

**Scenario:** Test avatar null và non-null cases

**Test Cases:**
1. **User có avatar trong DB:**
   - ✅ Server fetch avatar URL
   - ✅ `RoomPlayerState.avatar` = URL
   - ✅ Frontend hiển thị avatar từ URL

2. **User không có avatar (NULL):**
   - ✅ `RoomPlayerState.avatar` = ""
   - ✅ Frontend hiển thị default avatar (mushroom icon)

3. **Avatar URL invalid:**
   - ✅ Frontend img fallback đến default avatar

---

### TC-DATA-003: Database Persistence

**Scenario:** Verify DB sync với in-memory state

**Steps:**
1. User join phòng thành công
2. Query `room_members` table

**Expected Results:**
- ✅ Record mới trong `room_members`:
  ```sql
  SELECT * FROM room_members WHERE room_id=123 AND account_id=42;
  -- Returns: {room_id: 123, account_id: 42, joined_at: '2026-01-13...'}
  ```
- ✅ Nếu DB insert fail, game vẫn tiếp tục (eventual consistency)
- ✅ Log warning: `⚠️ DB insert failed (non-critical)`

---

## V. UI/UX TESTS

### TC-UI-001: Waiting Room Display

**Scenario:** Verify UI hiển thị đầy đủ thông tin

**Steps:**
1. User B join phòng của User A

**Expected Results - User B (Joiner):**
- ✅ Room title: "My Room 123"
- ✅ Room code: "ABC123" (hiển thị ở góc)
- ✅ Game rules panel:
  - Mode: "ELIMINATION" (active)
  - Max Players: 4
  - Wager Mode: ON (active badge)
  - Visibility: PUBLIC
- ✅ Member list panel:
  - Player 1: User A (with crown icon, "WAITING" status)
  - Player 2: User B ("WAITING" status)
  - 2 empty slots
  - Footer: "PLAYERS (2/4)"
- ✅ Action buttons:
  - "INVITE FRIENDS" (enabled)
  - "LEAVE ROOM" (enabled)
  - "START GAME" (disabled - not host)

**Expected Results - User A (Host):**
- ✅ Thấy User B xuất hiện trong member list
- ✅ "START GAME" button enabled khi all ready

---

### TC-UI-002: Error Message Display

**Scenario:** Test error feedback cho user

**Test Cases:**
1. **Room Full:**
   - ✅ Alert: "Phòng đã đầy"
   - ✅ User vẫn ở lobby

2. **Game Started:**
   - ✅ Alert: "Game đã bắt đầu"

3. **Invalid Code:**
   - ✅ Alert: "Mã phòng không hợp lệ"
   - ✅ Modal vẫn mở để user thử lại

4. **Network Error:**
   - ✅ Alert: "Không thể kết nối server"

---

## VI. PERFORMANCE TESTS

### TC-PERF-001: Join Latency

**Metric:** Thời gian từ click "Join" đến UI render

**Target:** < 500ms (local network)

**Steps:**
1. Measure time từ `CMD_JOIN_ROOM` sent đến `WaitingRoom` mounted

**Expected Results:**
- ✅ Network round-trip: < 100ms
- ✅ DB query (profiles): < 50ms
- ✅ React navigation + render: < 200ms
- ✅ Total: < 350ms

---

### TC-PERF-002: Concurrent Joins

**Scenario:** 4 users join cùng lúc vào 4 phòng khác nhau

**Expected Results:**
- ✅ Tất cả joins thành công
- ✅ Không có deadlock
- ✅ Mỗi join < 500ms

---

## VII. INTEGRATION TESTS

### TC-INT-001: End-to-End Flow

**Scenario:** Complete join flow từ UI đến DB

**Steps:**
1. User A tạo phòng
2. User B join từ public list
3. User C join bằng code
4. Verify tất cả layers

**Checkpoints:**
- ✅ **Frontend:** UI state correct
- ✅ **WebSocket:** Packets sent/received
- ✅ **Backend:** Logs show correct flow
- ✅ **Database:** `room_members` có 3 records
- ✅ **Broadcast:** Tất cả users thấy nhau

---

## VIII. REGRESSION TESTS

### TC-REG-001: Verify Old Functionality Still Works

**After implementing join_room, verify:**
- ✅ Create room vẫn hoạt động
- ✅ Leave room vẫn hoạt động
- ✅ Ready/Unready vẫn hoạt động
- ✅ Host transfer (khi host leave) vẫn hoạt động

---

## IX. MANUAL TEST CHECKLIST

### Pre-Test Setup
- [ ] Docker containers running (`docker-compose up`)
- [ ] Database có test data (ít nhất 2 accounts)
- [ ] Frontend dev server running
- [ ] Browser console open (để xem logs)
- [ ] Network tab open (để xem WebSocket packets)

### Test Execution
- [ ] TC-JOIN-001: Join public room ✅
- [ ] TC-JOIN-002: Join private room ✅
- [ ] TC-JOIN-003: Reject room full (elimination) ✅
- [ ] TC-JOIN-004: Reject room full (scoring) ✅
- [ ] TC-JOIN-005: Reject game started ✅
- [ ] TC-JOIN-006: Reject already in room ✅
- [ ] TC-JOIN-007: Reject private via public list ✅
- [ ] TC-JOIN-008: Reject invalid code ✅
- [ ] TC-RACE-001: Concurrent joins ✅
- [ ] TC-RACE-002: Player list timing ✅
- [ ] TC-DATA-001: Name caching ✅
- [ ] TC-DATA-002: Avatar handling ✅
- [ ] TC-UI-001: Waiting room display ✅
- [ ] TC-UI-002: Error messages ✅

### Post-Test Verification
- [ ] No errors in browser console
- [ ] No errors in server logs
- [ ] Database state consistent
- [ ] No memory leaks (check `docker stats`)

---

## X. BUG REPORT TEMPLATE

Nếu phát hiện lỗi, report theo format:

```
**Bug ID:** JOIN-BUG-XXX
**Severity:** Critical / High / Medium / Low
**Test Case:** TC-JOIN-XXX
**Environment:** Docker / Local / Production

**Steps to Reproduce:**
1. ...
2. ...

**Expected Result:**
...

**Actual Result:**
...

**Logs:**
```
[Paste relevant logs]
```

**Screenshots:**
[Attach if applicable]

**Root Cause (if known):**
...

**Suggested Fix:**
...
```

---

## XI. ACCEPTANCE CRITERIA

Chức năng JOIN_ROOM được coi là **PASS** khi:

✅ **Functional:**
- Tất cả happy path tests pass
- Tất cả error cases được handle đúng
- Race conditions được prevent

✅ **Data Integrity:**
- In-memory state sync với DB
- Player names và avatars cached correctly
- No data loss khi DB fail (eventual consistency)

✅ **Performance:**
- Join latency < 500ms
- Concurrent joins không gây deadlock

✅ **UX:**
- UI hiển thị đầy đủ thông tin
- Error messages rõ ràng
- No blank screens hoặc loading states vô hạn

✅ **Code Quality:**
- Logs đầy đủ để debug
- No memory leaks
- Code follows existing patterns

---

## XII. KNOWN LIMITATIONS

1. **Private Room Invite System:** Chưa implement (blocked tạm thời)
2. **Reconnect After Join:** Nếu user disconnect ngay sau join, cần test riêng
3. **Host Transfer:** Khi host leave, cần verify joiner có thể trở thành host

---

## XIII. FUTURE ENHANCEMENTS

1. **Automated Tests:** Viết integration tests với Jest/Cypress
2. **Load Testing:** Test với 100+ concurrent joins
3. **Monitoring:** Add metrics cho join success rate
4. **Analytics:** Track join sources (public list vs code)
