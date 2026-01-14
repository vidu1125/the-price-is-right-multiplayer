# Room Disconnection Handling - Version 2

## 📋 TÓM TẮT VẤN ĐỀ

### Hiện Trạng
Khi user đang ở **Waiting Room** bị disconnect (mất mạng, đóng tab, etc.), hệ thống **XÓA NGAY LẬP TỨC** khỏi room. User không thể rejoin vào room cũ, phải join lại từ đầu.

### Nguyên Nhân
- `room_handle_disconnect()` xóa player khỏi room ngay khi socket đóng
- `session_mark_disconnected()` chỉ set grace period cho `SESSION_PLAYING`, không xử lý `SESSION_LOBBY`
- Không có cơ chế "giữ slot" cho player disconnect tạm thời

---

## 🔍 PHÂN TÍCH KỸ THUẬT

### Flow Hiện Tại (Có Vấn Đề)

```
User disconnect → Socket close event
                ↓
    socket_server.c: Detect disconnect
                ↓
    if (state == SESSION_LOBBY):
        room_handle_disconnect()  ← XÓA NGAY
                ↓
    session_mark_disconnected()   ← KHÔNG LÀM GÌ (chỉ xử lý PLAYING)
                ↓
    Kết quả: User bị kick, không thể rejoin
```

### Code Hiện Tại

**File: `socket_server.c` (lines 196-207)**
```c
if (state == SESSION_LOBBY || state == SESSION_UNAUTHENTICATED) {
    room_handle_disconnect(fd, account_id);  // ← Xóa ngay
}
session_mark_disconnected(session);  // ← Không hoạt động với LOBBY
```

**File: `room_disconnect_handler.c`**
```c
void room_handle_disconnect(int client_fd, uint32_t account_id) {
    room_remove_member(room_id, client_fd);     // ← Xóa khỏi memory
    db_delete("room_members", ...);              // ← Xóa khỏi DB
    room_broadcast(room_id, NTF_PLAYER_LEFT);   // ← Thông báo đã rời
}
```

**File: `session_manager.c` (lines 145-151)**
```c
void session_mark_disconnected(UserSession *s) {
    if (s->state == SESSION_PLAYING) {  // ← CHỈ check PLAYING
        s->state = SESSION_PLAYING_DISCONNECTED;
        s->grace_deadline = time(NULL) + 300;
    }
    // ❌ SESSION_LOBBY → Không làm gì
}
```

---

## 💡 GIẢI PHÁP ĐỀ XUẤT

### Mục Tiêu
Cho phép user **reconnect vào room cũ trong 60 giây** nếu disconnect tạm thời.

### Chiến Lược
1. **KHÔNG XÓA** player khỏi room ngay lập tức
2. **MARK** player là `disconnected` (giữ slot)
3. **SET** grace period 60 giây
4. **CHO PHÉP** rejoin trong thời gian grace period
5. **XÓA** sau khi grace period hết

---

## 🛠️ IMPLEMENTATION PLAN

### File 1: `session_manager.c`
**Mục đích:** Thêm grace period cho SESSION_LOBBY

**Thay đổi hàm `session_mark_disconnected()`:**

```c
void session_mark_disconnected(UserSession *s) {
    if (!s) return;
    
    if (s->state == SESSION_PLAYING) {
        s->state = SESSION_PLAYING_DISCONNECTED;
        s->grace_deadline = time(NULL) + 300;  // 5 phút cho game
        printf("[SESSION] Player %u disconnected from game (grace: 5min)\n", 
               s->account_id);
    } 
    else if (s->state == SESSION_LOBBY) {
        // ✅ MỚI: Grace period cho waiting room
        s->grace_deadline = time(NULL) + 60;   // 1 phút cho waiting room
        printf("[SESSION] Player %u disconnected from lobby (grace: 1min)\n", 
               s->account_id);
        // Giữ nguyên state = SESSION_LOBBY
    }
}
```

**Lý do:**
- Không cần state mới `SESSION_LOBBY_DISCONNECTED`
- Chỉ cần set `grace_deadline` để track timeout
- Grace period ngắn hơn (60s vs 300s) vì chưa bắt đầu game

---

### File 2: `room_disconnect_handler.c`
**Mục đích:** Mark disconnected thay vì xóa ngay

**Thay đổi hàm `room_handle_disconnect()`:**

```c
void room_handle_disconnect(int client_fd, uint32_t account_id) {
    printf("========================================\n");
    printf("[ROOM_DISCONNECT] START\n");
    printf("[ROOM_DISCONNECT] client_fd=%d, account_id=%u\n", client_fd, account_id);
    
    // 1. Tìm room
    uint32_t room_id = room_find_by_player_fd(client_fd);
    if (room_id == 0) {
        printf("[ROOM_DISCONNECT] Player not in any room\n");
        printf("[ROOM_DISCONNECT] END\n");
        printf("========================================\n");
        return;
    }
    
    printf("[ROOM_DISCONNECT] Found player in room_id=%u\n", room_id);
    
    RoomState *room = room_get_state(room_id);
    if (!room) {
        printf("[ROOM_DISCONNECT] Room state not found\n");
        return;
    }
    
    // 2. ✅ MỚI: Mark disconnected (KHÔNG XÓA)
    bool found = false;
    for (int i = 0; i < room->player_count; i++) {
        if (room->member_fds[i] == client_fd) {
            // Mark as disconnected but keep in room
            room->players[i].connected = false;
            room->member_fds[i] = -1;  // Clear FD
            room->member_count--;       // Decrease active member count
            
            printf("[ROOM_DISCONNECT] Player %u marked as disconnected\n", account_id);
            printf("[ROOM_DISCONNECT] Grace period: 60 seconds\n");
            printf("[ROOM_DISCONNECT] Player slot preserved for reconnect\n");
            
            // Broadcast notification (player disconnected, not left)
            char notif[256];
            snprintf(notif, sizeof(notif), 
                     "{\"account_id\":%u,\"disconnected\":true}", account_id);
            room_broadcast(room_id, NTF_PLAYER_LEFT, notif, strlen(notif), -1);
            
            found = true;
            break;
        }
    }
    
    if (!found) {
        printf("[ROOM_DISCONNECT] Player FD not found in room\n");
    }
    
    // 3. ❌ KHÔNG XÓA khỏi database (giữ lại để rejoin)
    // 4. ❌ KHÔNG destroy room nếu rỗng (chờ grace period)
    
    printf("[ROOM_DISCONNECT] END\n");
    printf("========================================\n");
}
```

**Lý do:**
- Giữ player trong `room->players[]` array
- Chỉ mark `connected = false` và clear FD
- Không xóa khỏi DB để có thể rejoin

---

### File 3: `session_manager.c` (Cleanup Logic)
**Mục đích:** Xóa player sau khi grace period hết

**Thêm logic vào `session_cleanup_dead_sessions()`:**

```c
void session_cleanup_dead_sessions(void) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_SESSIONS; i++) {
        UserSession *s = &g_sessions[i];
        if (s->account_id == 0) continue;

        // ✅ MỚI: Cleanup cho SESSION_LOBBY với grace period
        if (s->state == SESSION_LOBBY && s->grace_deadline > 0) {
            if (now > s->grace_deadline) {
                printf("[SESSION] Grace timeout for lobby player %u\n", s->account_id);
                
                // Tìm room và xóa player
                uint32_t room_id = room_find_by_player_account(s->account_id);
                if (room_id > 0) {
                    printf("[SESSION] Removing player %u from room %u\n", 
                           s->account_id, room_id);
                    
                    // Xóa khỏi room state
                    room_remove_player_by_account(room_id, s->account_id);
                    
                    // Xóa khỏi database
                    char query[256];
                    snprintf(query, sizeof(query), 
                             "account_id = %u AND room_id = %u", 
                             s->account_id, room_id);
                    cJSON *response = NULL;
                    db_delete("room_members", query, &response);
                    if (response) cJSON_Delete(response);
                    
                    // Broadcast final leave notification
                    char notif[256];
                    snprintf(notif, sizeof(notif), 
                             "{\"account_id\":%u,\"timeout\":true}", s->account_id);
                    room_broadcast(room_id, NTF_PLAYER_LEFT, notif, strlen(notif), -1);
                }
                
                // Xóa session
                if (strlen(s->session_id) > 0) {
                    session_delete(s->session_id);
                }
                session_destroy(s);
            }
        }
        
        // Existing PLAYING_DISCONNECTED logic...
        else if (s->state == SESSION_PLAYING_DISCONNECTED) {
            if (s->grace_deadline && now > s->grace_deadline) {
                printf("[SESSION] Grace timeout → forfeit account_id=%d\n", s->account_id);
                room_remove_member_all(s->socket_fd);
                if (strlen(s->session_id) > 0) {
                    session_delete(s->session_id);
                }
                session_destroy(s);
            }
        }
    }
}
```

---

### File 4: `room_manager.h` + `room_manager.c`
**Mục đích:** Thêm helper functions

**Thêm vào `room_manager.h`:**
```c
// Find room by player account_id
uint32_t room_find_by_player_account(uint32_t account_id);

// Remove player by account_id (for cleanup after grace period)
void room_remove_player_by_account(uint32_t room_id, uint32_t account_id);
```

**Thêm vào `room_manager.c`:**
```c
uint32_t room_find_by_player_account(uint32_t account_id) {
    for (int i = 0; i < g_room_count; i++) {
        RoomState *room = &g_rooms[i];
        for (int j = 0; j < room->player_count; j++) {
            if (room->players[j].account_id == account_id) {
                return room->id;
            }
        }
    }
    return 0;
}

void room_remove_player_by_account(uint32_t room_id, uint32_t account_id) {
    RoomState *room = room_get_state(room_id);
    if (!room) return;
    
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].account_id == account_id) {
            // Shift remaining players
            for (int j = i; j < room->player_count - 1; j++) {
                room->players[j] = room->players[j + 1];
                room->member_fds[j] = room->member_fds[j + 1];
            }
            room->player_count--;
            
            printf("[ROOM] Removed player %u from room %u (timeout)\n", 
                   account_id, room_id);
            
            // Check if room is now empty
            if (room->player_count == 0) {
                room_close_if_empty(room_id);
            }
            return;
        }
    }
}
```

---

## 📊 SO SÁNH TRƯỚC/SAU

### Trước Khi Implement

| Scenario | Kết Quả |
|----------|---------|
| User disconnect 2 giây | ❌ Bị kick ngay, phải join lại |
| User disconnect 30 giây | ❌ Bị kick ngay, phải join lại |
| User disconnect 2 phút | ❌ Bị kick ngay, phải join lại |

### Sau Khi Implement

| Scenario | Kết Quả |
|----------|---------|
| User disconnect 2 giây | ✅ Reconnect thành công, vào lại room cũ |
| User disconnect 30 giây | ✅ Reconnect thành công, vào lại room cũ |
| User disconnect 2 phút | ❌ Grace period hết (60s), bị xóa khỏi room |

---

## 🧪 TEST CASES

### TC-1: Disconnect và Reconnect Trong 60s
**Steps:**
1. User A tạo room, User B join
2. User B disconnect (đóng tab)
3. Đợi 10 giây
4. User B login lại

**Expected:**
- ✅ User B vẫn thấy mình trong room
- ✅ Room vẫn có 2 players
- ✅ User A thấy User B reconnect

### TC-2: Disconnect Quá 60s
**Steps:**
1. User A tạo room, User B join
2. User B disconnect
3. Đợi 70 giây
4. User B login lại

**Expected:**
- ✅ User B bị xóa khỏi room sau 60s
- ✅ Room chỉ còn User A
- ✅ User B phải join lại từ đầu

### TC-3: Host Disconnect
**Steps:**
1. User A (host) tạo room, User B join
2. User A disconnect
3. Đợi 30 giây
4. User A reconnect

**Expected:**
- ✅ User A vẫn là host
- ✅ Room không bị đóng
- ✅ User B vẫn ở trong room

---

## ⚠️ TRADE-OFFS

### Ưu Điểm
- ✅ UX tốt hơn: User không bị kick do mất mạng tạm thời
- ✅ Giảm frustration: Không phải join lại từ đầu
- ✅ Consistent với game logic (PLAYING đã có grace period)

### Nhược Điểm
- ⚠️ Phức tạp hơn: Thêm state tracking
- ⚠️ Memory overhead: Giữ disconnected players trong 60s
- ⚠️ Edge cases: Cần handle host disconnect, room empty, etc.

### Rủi Ro
- 🔴 **Race condition:** User reconnect đúng lúc grace period hết
- 🔴 **Zombie rooms:** Room có toàn disconnected players
- 🟡 **DB inconsistency:** In-memory vs DB state mismatch

---

## 🚀 ROLLOUT PLAN

### Phase 1: Implementation (2-3 giờ)
1. ✅ Sửa `session_manager.c` - Grace period logic
2. ✅ Sửa `room_disconnect_handler.c` - Mark disconnected
3. ✅ Sửa `session_manager.c` - Cleanup logic
4. ✅ Thêm helper functions vào `room_manager.c`

### Phase 2: Testing (1-2 giờ)
1. ✅ Unit test: Grace period timeout
2. ✅ Integration test: Reconnect flow
3. ✅ Edge case test: Host disconnect, room empty

### Phase 3: Deployment
1. ✅ Code review với team
2. ✅ Merge vào main branch
3. ✅ Deploy lên staging
4. ✅ Monitor logs và metrics

---

## 📝 NOTES CHO TEAM DISCUSSION

### Câu Hỏi Cần Thảo Luận
1. **Grace period duration:** 60s có hợp lý không? Hay nên 30s hoặc 90s?
2. **Host disconnect:** Có cần transfer host ngay hay đợi grace period?
3. **Empty room:** Room toàn disconnected players có nên đóng ngay không?
4. **Notification:** Frontend cần notification gì khi player disconnect/reconnect?
5. **Database sync:** Có cần update `room_members.connected` column không?

### Alternative Approaches
1. **Approach A (Hiện tại):** Grace period 60s, giữ slot
2. **Approach B (Aggressive):** Grace period 30s, kick nhanh hơn
3. **Approach C (Lenient):** Grace period 120s, cho phép reconnect lâu hơn
4. **Approach D (No grace):** Giữ nguyên hiện tại, kick ngay

### Recommendation
**Implement Approach A** với grace period 60s vì:
- Balance giữa UX và resource usage
- Consistent với industry standard (most games: 30-90s)
- Đủ thời gian cho network hiccup, không quá lâu gây zombie rooms

---

## 📚 REFERENCES

- Session management pattern: [Link to session_manager.c]
- Room lifecycle: [Link to room_manager.c]
- Disconnect handling: [Link to socket_server.c]
- Related issue: [Link to GitHub issue nếu có]
