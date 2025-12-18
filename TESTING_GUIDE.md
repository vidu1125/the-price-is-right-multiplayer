# 🧪 TESTING GUIDE - HOST FEATURES

## 📋 CHIẾN LƯỢC TEST

### **3 Mức độ test:**
1. **Backend Layer** - Test socket client độc lập
2. **Integration** - Test C Client → C Server
3. **Full Stack** - Test React UI → Backend → C Server

---

## ⚙️ CHUẨN BỊ

### **1. Build các services:**

```bash
# Build C Server
cd Network/server
make clean && make

# Build C Client
cd Network/client
make clean && make
```

### **2. Start services (3 terminals):**

**Terminal 1 - C Server:**
```bash
cd Network/server
./build/server

# Expected output:
# ═══════════════════════════════════════
#   The Price Is Right - Socket Server
#   Network Layer (C)
# ═══════════════════════════════════════
# ✅ Listening on port 8888
# Ready to accept connections...
```

**Terminal 2 - Python Backend:**
```bash
cd Backend
python app/main.py

# Expected output:
# 🚀 Backend API starting on port 5000...
# ✅ Database connected
# 🔌 Starting IPC server...
# [IPC] Server listening on /tmp/tpir_backend.sock
```

**Terminal 3 - Tests:**
```bash
# Sẽ dùng terminal này để chạy tests
```

---

## 🔬 PHASE 1: TEST BACKEND LAYER

### **Test 1.1: Python Socket Client → C Server**

```bash
cd Backend
python test_socket_client.py
```

**Mục đích:** Kiểm tra RAW TCP SOCKET communication

**Expected output:**
```
╔════════════════════════════════════════════════════════╗
║  SOCKET CLIENT TEST - RAW TCP SOCKET TO C SERVER      ║
╚════════════════════════════════════════════════════════╝

============================================================
TEST 1: CREATE ROOM
============================================================
[Socket] Connected to C Server localhost:8888
📤 Sent command 0x0200, payload size: 72

✅ Result: {'success': True, 'room_id': 123, 'room_code': 'ABC123', 'host_id': 1}
   Room ID: 123
   Room Code: ABC123
   Host ID: 1

============================================================
TEST 2: SET RULES
============================================================
...
```

**Xem logs:**
- **C Server terminal:** Thấy connection accepted, packet received
- **Backend terminal:** IPC requests được route

**Điều kiện PASS:**
- ✅ All 5 tests return `{'success': True}`
- ✅ C Server logs show incoming connections
- ✅ Backend logs show IPC communication

---

## 🖥️ PHASE 2: TEST C TERMINAL CLIENT

### **Test 2.1: C Client → C Server (Raw Socket)**

```bash
cd Network/client
./build/client
```

**Test từng use case:**

**Use Case 1: Create Room**
```
Enter choice: 1
=== CREATE ROOM ===
Room Name: My Test Room
Visibility (0=public, 1=private): 0
Mode (0=scoring, 1=elimination): 0
Max Players (4-6): 4
Round Time (15-60 seconds): 30
Enable Wager (0=no, 1=yes): 0

📤 Sent command 0x0200, payload: 72 bytes
📥 Response received: command=0x00CC, payload=27 bytes

✅ Room created successfully!
   Room ID: 124
   Room Code: XYZ789
   Host ID: 1
```

**Use Case 2: Set Rules**
```
Enter choice: 2
=== SET RULES ===
Room ID: 124
Mode (0=scoring, 1=elimination): 1
Max Players (4-6): 6
Round Time (15-60): 60
Enable Wager (0/1): 1

📤 Sent command 0x0206, payload: 16 bytes
📥 Response received: command=0x00CA, payload=0 bytes

✅ Rules updated successfully!
```

**Use Case 3: Kick Member** (sẽ fail vì không có member)
```
Enter choice: 3
=== KICK MEMBER ===
Room ID: 124
Target User ID to kick: 999

📤 Sent command 0x0204, payload: 8 bytes
📥 Response received: command=0x00CB, payload=0 bytes

❌ Failed to kick member
(Expected - member 999 không tồn tại)
```

**Use Case 4: Start Game**
```
Enter choice: 5
=== START GAME ===
Room ID: 124

📤 Sent command 0x0300, payload: 4 bytes
📥 Response received: command=0x00CD, payload=24 bytes

✅ Game starting!
   Match ID: 456
   Countdown: 3000 ms
   Server Time: 1702886400000 ms
   Game Start Time: 1702886403000 ms
```

**Use Case 5: Delete Room**
```
Enter choice: 4
=== DELETE ROOM ===
Room ID: 124

📤 Sent command 0x0207, payload: 4 bytes
📥 Response received: command=0x00CA, payload=0 bytes

✅ Room deleted successfully!
```

**Điều kiện PASS:**
- ✅ Create Room trả về room_id và room_code
- ✅ Set Rules thành công
- ✅ Kick Member fail như expected (không có member)
- ✅ Start Game trả về match_id và timestamps
- ✅ Delete Room thành công

---

## 🌐 PHASE 3: TEST HTTP API

### **Test 3.1: curl commands**

```bash
# Test Create Room
curl -X POST http://localhost:5000/api/network/command \
  -H "Content-Type: application/json" \
  -d '{
    "command": "CREATE_ROOM",
    "account_id": 1,
    "data": {
      "room_name": "HTTP Test",
      "visibility": 0,
      "mode": 0,
      "max_players": 4,
      "round_time": 30,
      "wager_enabled": 0
    }
  }' | jq '.'
```

**Expected response:**
```json
{
  "success": true,
  "room": {
    "room_id": 125,
    "room_code": "DEF456",
    "host_id": 1,
    "created_at": 1702886400000
  }
}
```

**Test các commands khác tương tự:**

```bash
# Set Rules
curl -X POST http://localhost:5000/api/network/command \
  -H "Content-Type: application/json" \
  -d '{
    "command": "SET_RULE",
    "account_id": 1,
    "data": {
      "room_id": 125,
      "mode": 1,
      "max_players": 6,
      "round_time": 60,
      "wager_enabled": 1
    }
  }' | jq '.'

# Start Game
curl -X POST http://localhost:5000/api/network/command \
  -H "Content-Type: application/json" \
  -d '{
    "command": "START_GAME",
    "account_id": 1,
    "data": {"room_id": 125}
  }' | jq '.'

# Delete Room
curl -X POST http://localhost:5000/api/network/command \
  -H "Content-Type: application/json" \
  -d '{
    "command": "DELETE_ROOM",
    "account_id": 1,
    "data": {"room_id": 125}
  }' | jq '.'
```

**Điều kiện PASS:**
- ✅ All requests return HTTP 200
- ✅ Response có `"success": true`
- ✅ Data structure đúng format

---

## 🎨 PHASE 4: TEST REACT UI

### **Test 4.1: Full Stack End-to-End**

```bash
# Terminal 4
cd Frontend
npm start

# Browser: http://localhost:3000
```

**Test flow:**

**1. Create Room:**
- Go to `/lobby`
- Click "Create new room"
- Fill form:
  - Name: "UI Test Room"
  - Visibility: Public
  - Mode: Scoring
  - Max Players: 4
  - Round Time: 30
  - Wager: Unchecked
- Click "Create Room"
- **Expected:** Alert "Room created! Code: ABC123", modal closes

**2. Set Rules:**
- In WaitingRoom
- Click "edit" button on Game Rules panel
- Change:
  - Mode: Elimination
  - Max Players: 6
  - Wager: ON
  - Round Time: Fast (60s)
- Click "Save"
- **Expected:** Alert "Rules updated successfully!"

**3. Kick Member:** (skip nếu không có member thật)

**4. Start Game:**
- Click "START GAME" button
- Confirm dialog
- **Expected:**
  - Full-screen countdown overlay (3... 2... 1...)
  - Navigate to `/game/:matchId`

**5. Delete Room:**
- Back to WaitingRoom (tạo room mới)
- Click "DELETE ROOM"
- Confirm dialog
- **Expected:**
  - Navigate to `/lobby`
  - Room không còn trong room list

**Điều kiện PASS:**
- ✅ Tất cả actions hiển thị đúng kết quả
- ✅ Countdown animation hoạt động
- ✅ Navigation đúng route
- ✅ No console errors

---

## 📊 MONITORING LOGS

### **Xem logs để debug:**

**C Server logs:**
```
[2025-12-18 10:30:15] Client connected from 127.0.0.1:54321
[2025-12-18 10:30:15] Received packet: cmd=0x0200, len=72
[2025-12-18 10:30:15] Forwarding to backend via IPC...
[2025-12-18 10:30:15] Backend response received
[2025-12-18 10:30:15] Sent response: cmd=0x00CC, len=27
```

**Python Backend logs:**
```
[2025-12-18 10:30:15] [Socket] Connected to C Server localhost:8888
[2025-12-18 10:30:15] [Socket] Sending command 0x0200, payload size: 72
[2025-12-18 10:30:15] [Socket] Response command: 0x00CC, payload: 27 bytes
[2025-12-18 10:30:15] [Network] Command received: CREATE_ROOM from user 1
[2025-12-18 10:30:15] [Network] Forwarding to C Server via RAW TCP SOCKET...
```

**Browser Console (React):**
```
[Socket] Connected, session: session_1
POST http://localhost:5000/api/network/command 200 OK (125ms)
{success: true, room: {room_id: 125, room_code: "ABC123", ...}}
```

---

## ✅ CHECKLIST TỔNG KẾT

### **Backend Layer:**
- [ ] Python socket_client.py test PASS (5/5 tests)
- [ ] C Server nhận được binary packets
- [ ] Backend IPC communication hoạt động

### **C Client:**
- [ ] Create Room thành công
- [ ] Set Rules thành công
- [ ] Kick Member (expected fail OK)
- [ ] Start Game với countdown timestamps
- [ ] Delete Room thành công

### **HTTP API:**
- [ ] All curl commands return 200
- [ ] JSON responses đúng format
- [ ] Logs show socket communication

### **React UI:**
- [ ] Create Room modal hoạt động
- [ ] Set Rules edit mode hoạt động
- [ ] Start Game countdown animation
- [ ] Delete Room navigation đúng
- [ ] No console errors

---

## 🚀 QUICK START (Automated)

```bash
# Chạy toàn bộ test plan
./test_plan.sh
```

Script này sẽ:
1. Build C Server và C Client
2. Hướng dẫn start services
3. Chạy Python socket tests
4. Hướng dẫn test C Client
5. Test HTTP API với curl
6. Hướng dẫn test React UI

---

## 🎯 KẾT LUẬN

**Nếu tất cả tests PASS:**
- ✅ RAW TCP SOCKET communication hoạt động đúng
- ✅ Binary protocol được encode/decode chính xác
- ✅ HTTP → Socket → C Server → IPC → Backend flow hoàn chỉnh
- ✅ UI integration thành công

**Sẵn sàng cho demo/submission! 🎉**
