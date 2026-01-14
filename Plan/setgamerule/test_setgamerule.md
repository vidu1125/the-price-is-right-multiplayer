# TEST SCENARIO: SET GAME RULE

**Feature:** Host thay đổi cấu hình phòng trước khi bắt đầu game  
**Date:** 2026-01-14  
**Status:** ✅ Ready for Testing

---

## 🎯 **TEST OBJECTIVES**

Verify that:
1. ✅ Host có thể thay đổi rules (mode, max_players, visibility, wager_mode)
2. ✅ Non-host không thể thay đổi rules
3. ✅ Validation rules hoạt động đúng
4. ✅ Ready states được reset sau khi rules thay đổi
5. ✅ Tất cả players nhận được notifications
6. ✅ Database được sync đúng

---

## 🧪 **TEST CASES**

### **TC-1: Host Thay Đổi Rules Thành Công (Happy Path)**

**Precondition:**
- User A (Host) đã tạo room với:
  - mode: `elimination`
  - max_players: `4`
  - visibility: `public`
  - wager_mode: `false`
- User B đã join vào room
- Cả 2 users đều đã ready (`is_ready = true`)

**Test Steps:**
1. Host click nút **"edit"** trên Game Rules Panel
2. Thay đổi mode: `elimination` → `scoring`
3. Thay đổi max_players: `4` → `6`
4. Thay đổi visibility: `public` → `private`
5. Thay đổi wager_mode: `false` → `true`
6. Click nút **"done"**

**Expected Results:**
- ✅ **Frontend (Host):**
  - Nhận `RES_RULES_UPDATED` (0x00E0)
  - Console log: `✅ Game rules committed`
  - Edit mode tắt
  
- ✅ **Frontend (All Players):**
  - Nhận `NTF_RULES_CHANGED` (0x02C9) với payload:
    ```json
    {
      "mode": "scoring",
      "maxPlayers": 6,
      "visibility": "private",
      "wagerMode": true
    }
    ```
  - Nhận `NTF_PLAYER_LIST` (0x02BE) với tất cả `is_ready = false`
  - UI Game Rules Panel cập nhật hiển thị rules mới
  - Ready buttons reset về trạng thái "Not Ready"
  
- ✅ **Backend:**
  - Server logs:
    ```
    [SetGameRule] BEFORE changes:
      - mode: 0 (ELIMINATION)
      - max_players: 4
      - visibility: 0 (PUBLIC)
      - wager_mode: 0
    
    [SetGameRule] AFTER changes:
      - mode: 1 (SCORING)
      - max_players: 6
      - visibility: 1 (PRIVATE)
      - wager_mode: 1
    
    [SetGameRule] Resetting ready states for 2 players:
      [0] Alice (id=7): ready true -> false
      [1] Bob (id=8): ready true -> false
    
    [SetGameRule] ✅ DB sync successful
    [SetGameRule] ✅ SUCCESS: room_id=1, all players notified
    ```
  
- ✅ **Database:**
  - Table `rooms` updated:
    ```sql
    SELECT mode, max_players, visibility, wager_mode 
    FROM rooms WHERE id = 1;
    -- Result: scoring, 6, private, true
    ```

---

### **TC-2: Non-Host Không Thể Thay Đổi Rules**

**Precondition:**
- User A (Host) đã tạo room
- User B (Member) đã join vào room

**Test Steps:**
1. User B (không phải host) cố gắng gửi `CMD_SET_RULE` packet

**Expected Results:**
- ✅ Server trả về `ERR_NOT_HOST` (406)
- ✅ Error message: `"Only host can change rules"`
- ✅ Rules không thay đổi
- ✅ Database không bị update

**How to Test:**
```bash
# Sử dụng Postman hoặc curl để gửi packet trực tiếp
# với session của User B (non-host)
```

---

### **TC-3: Validation - Elimination Mode Phải Có Đúng 4 Players**

**Precondition:**
- Host đã tạo room với mode `scoring`, max_players `5`

**Test Steps:**
1. Host click "edit"
2. Chọn mode: `elimination`
3. Giữ nguyên max_players: `5` (không đổi)
4. Click "done"

**Expected Results:**
- ✅ Server trả về `ERR_BAD_REQUEST` (400)
- ✅ Error message: `"Elimination mode requires exactly 4 players"`
- ✅ Rules không thay đổi
- ✅ UI vẫn hiển thị rules cũ

---

### **TC-4: Validation - Scoring Mode Phải Có 4-6 Players**

**Test 4a: max_players = 3 (Invalid)**

**Steps:**
1. Host chọn mode `scoring`, max_players `3`
2. Click "done"

**Expected:**
- ✅ `ERR_BAD_REQUEST` (400)
- ✅ Message: `"Scoring mode requires 4-6 players"`

**Test 4b: max_players = 7 (Invalid)**

**Steps:**
1. Host chọn mode `scoring`, max_players `7`
2. Click "done"

**Expected:**
- ✅ `ERR_BAD_REQUEST` (400)
- ✅ Message: `"Scoring mode requires 4-6 players"`

---

### **TC-5: Validation - Max Players < Current Players**

**Precondition:**
- Room hiện có 5 players (Host + 4 members)
- Current max_players: `6`

**Test Steps:**
1. Host click "edit"
2. Thay đổi max_players: `6` → `4`
3. Click "done"

**Expected Results:**
- ✅ Server trả về `ERR_BAD_REQUEST` (400)
- ✅ Error message: 
  ```
  "Cannot set max players to 4. Room currently has 5 players. 
   Please kick players first or choose a higher limit."
  ```
- ✅ Rules không thay đổi
- ✅ Frontend hiển thị error message cho host

---

### **TC-6: Room Status Validation**

**Precondition:**
- Room đã bắt đầu game (`status = ROOM_PLAYING`)

**Test Steps:**
1. Host cố gắng thay đổi rules

**Expected Results:**
- ✅ Server trả về `ERR_BAD_REQUEST` (400)
- ✅ Error message: `"Cannot change rules after game started"`
- ✅ Rules không thay đổi

---

### **TC-7: Database Sync Failure (Edge Case)**

**Precondition:**
- PostgreSQL database bị disconnect hoặc slow

**Test Steps:**
1. Stop PostgreSQL: `docker-compose stop postgres`
2. Host thay đổi rules và click "done"

**Expected Results:**
- ✅ In-memory `RoomState` vẫn được update
- ✅ Host nhận `RES_RULES_UPDATED`
- ✅ All players nhận `NTF_RULES_CHANGED` và `NTF_PLAYER_LIST`
- ✅ Server log warning:
  ```
  [SetGameRule] ⚠️  Warning: Failed to sync rules to DB
  [SetGameRule] ✅ SUCCESS: room_id=1, all players notified
  ```
- ✅ Game vẫn tiếp tục hoạt động (eventual consistency)

**Cleanup:**
```bash
docker-compose start postgres
# Rules sẽ được sync lại khi DB available
```

---

### **TC-8: Broadcast To All Players**

**Precondition:**
- Room có 4 players: Host (A), Member B, Member C, Member D

**Test Steps:**
1. Host thay đổi rules và click "done"

**Expected Results:**
- ✅ **Host (A):** Nhận `RES_RULES_UPDATED` + `NTF_RULES_CHANGED` + `NTF_PLAYER_LIST`
- ✅ **Member B:** Nhận `NTF_RULES_CHANGED` + `NTF_PLAYER_LIST`
- ✅ **Member C:** Nhận `NTF_RULES_CHANGED` + `NTF_PLAYER_LIST`
- ✅ **Member D:** Nhận `NTF_RULES_CHANGED` + `NTF_PLAYER_LIST`

**Verification:**
- Check browser console logs cho tất cả 4 clients
- Verify tất cả đều thấy rules mới và ready states reset

---

### **TC-9: Ready State Reset**

**Precondition:**
- Room có 3 players
- Player states:
  - Host (A): `is_ready = true`
  - Member B: `is_ready = true`
  - Member C: `is_ready = false`

**Test Steps:**
1. Host thay đổi rules và click "done"

**Expected Results:**
- ✅ Server logs:
  ```
  [SetGameRule] Resetting ready states for 3 players:
    [0] Alice (id=7): ready true -> false
    [1] Bob (id=8): ready true -> false
    [2] Charlie (id=9): ready false -> false
  ```
- ✅ Tất cả 3 players nhận `NTF_PLAYER_LIST` với `is_ready = false`
- ✅ UI hiển thị tất cả ready buttons ở trạng thái "Not Ready"

---

### **TC-10: Multiple Rule Changes**

**Precondition:**
- Room mới tạo với default rules

**Test Steps:**
1. Host thay đổi rules lần 1: mode `elimination` → `scoring`
2. Click "done"
3. Đợi notifications
4. Host click "edit" lại
5. Thay đổi rules lần 2: max_players `4` → `6`
6. Click "done"

**Expected Results:**
- ✅ Mỗi lần click "done" trigger 1 round notifications
- ✅ Rules được update đúng sau mỗi lần
- ✅ Ready states reset sau mỗi lần
- ✅ Database sync đúng

---

## 📊 **VALIDATION MATRIX**

| Mode | Max Players | Current Players | Valid? | Error Message |
|------|-------------|-----------------|--------|---------------|
| Elimination | 4 | 2 | ✅ | - |
| Elimination | 4 | 4 | ✅ | - |
| Elimination | 5 | 2 | ❌ | "Elimination mode requires exactly 4 players" |
| Elimination | 6 | 3 | ❌ | "Elimination mode requires exactly 4 players" |
| Scoring | 3 | 2 | ❌ | "Scoring mode requires 4-6 players" |
| Scoring | 4 | 3 | ✅ | - |
| Scoring | 5 | 4 | ✅ | - |
| Scoring | 6 | 5 | ✅ | - |
| Scoring | 7 | 4 | ❌ | "Scoring mode requires 4-6 players" |
| Scoring | 4 | 5 | ❌ | "Cannot set max players to 4. Room currently has 5 players..." |

---

## 🔍 **MANUAL TESTING CHECKLIST**

### **Setup:**
```bash
# 1. Start services
docker-compose up --build -d

# 2. Open 2 browser windows
# Window 1: localhost:3000 (Host - User A)
# Window 2: localhost:3000 (Member - User B)

# 3. Create accounts and login
```

### **Test Execution:**
- [ ] **TC-1:** Happy path - thay đổi tất cả fields
- [ ] **TC-2:** Non-host không thể thay đổi
- [ ] **TC-3:** Elimination mode validation
- [ ] **TC-4:** Scoring mode validation
- [ ] **TC-5:** Max players < current players
- [ ] **TC-6:** Room status validation
- [ ] **TC-7:** Database sync failure
- [ ] **TC-8:** Broadcast to all players
- [ ] **TC-9:** Ready state reset
- [ ] **TC-10:** Multiple rule changes

### **Verification Points:**
- [ ] Check browser console logs (both windows)
- [ ] Check server logs: `docker-compose logs -f network`
- [ ] Check database: 
  ```sql
  SELECT * FROM rooms WHERE id = <room_id>;
  ```
- [ ] Verify UI updates correctly
- [ ] Verify ready buttons reset

---

## 🐛 **KNOWN ISSUES / NOTES**

1. **Frontend UI:** Nếu host thay đổi mode từ `scoring` → `elimination`, max_players tự động set về 4 (đúng theo logic)
2. **Database Sync:** Nếu DB fail, game vẫn chạy được (eventual consistency)
3. **Notification Order:** `NTF_RULES_CHANGED` được gửi trước `NTF_PLAYER_LIST`

---

## ✅ **SUCCESS CRITERIA**

Tất cả test cases phải PASS:
- ✅ Validation rules hoạt động đúng
- ✅ Broadcast đến tất cả players
- ✅ Ready states reset đúng
- ✅ Database sync (hoặc log warning nếu fail)
- ✅ UI cập nhật đúng
- ✅ No crashes, no memory leaks

---

## 📝 **TEST REPORT TEMPLATE**

```markdown
## Test Execution Report

**Date:** YYYY-MM-DD  
**Tester:** [Your Name]  
**Environment:** Docker Compose (PostgreSQL + Network + Frontend)

### Results:
| Test Case | Status | Notes |
|-----------|--------|-------|
| TC-1 | ✅ PASS | - |
| TC-2 | ✅ PASS | - |
| TC-3 | ✅ PASS | - |
| TC-4 | ✅ PASS | - |
| TC-5 | ✅ PASS | - |
| TC-6 | ✅ PASS | - |
| TC-7 | ✅ PASS | - |
| TC-8 | ✅ PASS | - |
| TC-9 | ✅ PASS | - |
| TC-10 | ✅ PASS | - |

### Issues Found:
- None

### Recommendations:
- Feature ready for production
```
