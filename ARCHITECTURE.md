# ARCHITECTURE SUMMARY - HOST FEATURES IMPLEMENTATION

## 🎯 TUÂN THỦ APPLICATION DESIGN

Kiến trúc được thiết kế theo yêu cầu:
- ✅ **Client-Server:** Sử dụng RAW TCP SOCKET với Binary Protocol
- ✅ **Frontend-Backend:** HTTP/JSON (vì browser không hỗ trợ raw socket)

---

## 🏗️ KIẾN TRÚC 2 LUỒNG

### **LUỒNG 1: C Client (Terminal Application) - ĐÁNH GIÁ MÔN HỌC**

```
┌─────────────────────────────────────────────────┐
│  C TERMINAL CLIENT                              │
│  File: Network/client/src/client_main.c        │
│  - Menu terminal (1-5: host features)          │
│  - Raw TCP socket programming                  │
│  - Binary protocol encoding/decoding           │
└────────────┬────────────────────────────────────┘
             │
             │ RAW TCP SOCKET
             │ Binary Protocol (16-byte header + payload)
             │ connect(localhost, 8888)
             │ send() / recv()
             ↓
┌─────────────────────────────────────────────────┐
│  C SERVER                                       │
│  File: Network/server/src/main.c               │
│  - select() I/O multiplexing                   │
│  - Parse binary packets                        │
│  - Route commands                              │
└────────────┬────────────────────────────────────┘
             │
             │ IPC (Unix Socket)
             │ JSON-RPC over /tmp/tpir_backend.sock
             ↓
┌─────────────────────────────────────────────────┐
│  PYTHON BACKEND - Business Logic               │
│  Files: Backend/app/services/host/*.py         │
│  - Validate input                              │
│  - Database operations                         │
│  - Game engine                                 │
└────────────┬────────────────────────────────────┘
             │
             │ SQL (psycopg2)
             ↓
┌─────────────────────────────────────────────────┐
│  SUPABASE POSTGRESQL                            │
│  Tables: accounts, rooms, matches...           │
└─────────────────────────────────────────────────┘
```

**Cách test:**
```bash
# Terminal 1: Start C Server
cd Network/server
make && ./build/server

# Terminal 2: Start Python Backend
cd Backend
python app/main.py

# Terminal 3: Run C Client
cd Network/client
make && ./build/client
# Chọn menu 1-5 để test các host features
```

---

### **LUỒNG 2: React UI (Web Application) - DEMO/PRODUCTION**

```
┌─────────────────────────────────────────────────┐
│  REACT FRONTEND (Browser)                      │
│  Files: Frontend/src/components/**/*.js        │
│  - CreateRoomPanel, WaitingRoom, etc.         │
│  - socketService.js (HTTP client)             │
└────────────┬────────────────────────────────────┘
             │
             │ HTTP POST
             │ http://localhost:5000/api/network/command
             │ Content-Type: application/json
             │ Body: {"command": "CREATE_ROOM", "data": {...}}
             ↓
┌─────────────────────────────────────────────────┐
│  PYTHON BACKEND - HTTP Proxy                   │
│  File: Backend/app/routes/network_routes.py    │
│  - Receive HTTP JSON request                   │
│  - Convert JSON → Binary Protocol              │
└────────────┬────────────────────────────────────┘
             │
             │ RAW TCP SOCKET
             │ File: Backend/app/services/socket_client.py
             │ socket.connect(('localhost', 8888))
             │ socket.send(binary_packet)
             ↓
┌─────────────────────────────────────────────────┐
│  C SERVER (SAME AS ABOVE)                      │
│  - Receive binary packet                       │
│  - Parse and route                             │
│  - Forward to Python Backend via IPC           │
│  - Return binary response                      │
└────────────┬────────────────────────────────────┘
             │
             │ Binary response
             ↑
┌─────────────────────────────────────────────────┐
│  PYTHON BACKEND - HTTP Proxy (RECEIVE)         │
│  - Parse binary response                       │
│  - Convert Binary → JSON                       │
│  - Return HTTP JSON response                   │
└────────────┬────────────────────────────────────┘
             │
             │ HTTP 200 OK
             │ Content-Type: application/json
             │ Body: {"success": true, "room": {...}}
             ↓
┌─────────────────────────────────────────────────┐
│  REACT FRONTEND (Display result)               │
└─────────────────────────────────────────────────┘
```

**Cách test:**
```bash
# Terminal 1: Start C Server
cd Network/server
make && ./build/server

# Terminal 2: Start Python Backend
cd Backend
python app/main.py

# Terminal 3: Start React Dev Server
cd Frontend
npm start

# Browser: http://localhost:3000
# Test UI: Click "Create Room", điền form, submit
```

---

## 📦 FILES STRUCTURE

```
Project/
├── Frontend/
│   ├── src/
│   │   ├── services/
│   │   │   └── socketService.js          ✅ HTTP client
│   │   └── components/
│   │       ├── Lobby/
│   │       │   └── CreateRoomPanel.js    ✅ Create Room UI
│   │       └── Room/WaitingRoom/
│   │           ├── GameRulesPanel.js     ✅ Set Rules UI
│   │           ├── MemberListPanel.js    ✅ Kick Member UI
│   │           └── WaitingRoom.js        ✅ Start/Delete UI
│   └── package.json
│
├── Backend/
│   ├── app/
│   │   ├── routes/
│   │   │   └── network_routes.py         ✅ HTTP → Socket proxy
│   │   ├── services/
│   │   │   ├── socket_client.py          ✅ RAW TCP Socket client
│   │   │   └── host/
│   │   │       ├── room_service.py       ✅ Business logic
│   │   │       ├── rule_service.py
│   │   │       └── member_service.py
│   │   └── ipc/
│   │       └── __init__.py               ✅ IPC server
│   └── requirements.txt
│
└── Network/
    ├── client/
    │   ├── src/
    │   │   └── client_main.c             ✅ C Terminal client
    │   └── Makefile                      ✅ Build config
    │
    ├── server/
    │   ├── src/
    │   │   ├── main.c                    ✅ C Server (select loop)
    │   │   ├── router.c                  ✅ Command routing
    │   │   ├── packet_handler.c          ✅ Binary I/O
    │   │   └── backend_bridge.c          ✅ IPC client
    │   └── Makefile
    │
    └── protocol/
        ├── commands.h                    ✅ Command codes
        ├── packet_format.h               ✅ Header format
        └── payloads.h                    ✅ Payload structures
```

---

## 🔑 KEY POINTS

### 1. **RAW TCP SOCKET - Client to Server**

**C Client (Terminal):**
```c
// Network/client/src/client_main.c
int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
connect(sock_fd, &server_addr, sizeof(server_addr));

// Build binary packet
PacketHeader header;
header.magic = htons(0x4347);
header.command = htons(CMD_CREATE_ROOM);
// ...

send(sock_fd, &header, sizeof(header), 0);
send(sock_fd, &payload, payload_size, 0);

recv(sock_fd, &response_header, sizeof(PacketHeader), 0);
recv(sock_fd, &response_payload, payload_size, 0);
```

**Python Backend (Proxy):**
```python
# Backend/app/services/socket_client.py
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 8888))

# Build binary packet
header = struct.pack('!HHBBHHII', 
    0x4347, 0x01, 0x00, command, 0, seq, len(payload))

sock.sendall(header + payload)
response = sock.recv(4096)
```

### 2. **Binary Protocol Format**

**Header (16 bytes):**
```c
typedef struct {
    uint16_t magic;           // 0x4347 ('CG')
    uint16_t version;         // 0x0001
    uint8_t  flags;
    uint8_t  command_high;
    uint16_t command;         // CMD_CREATE_ROOM, etc.
    uint16_t reserved;
    uint32_t sequence;
    uint32_t payload_length;
} PacketHeader;
```

**Example: CREATE_ROOM Payload (72 bytes):**
```c
typedef struct {
    char     room_name[64];
    uint8_t  visibility;      // 0: public, 1: private
    uint8_t  mode;            // 0: scoring, 1: elimination
    uint8_t  max_players;     // 4-6
    uint16_t round_time;      // 15-60 seconds
    uint8_t  wager_enabled;
    uint8_t  padding[2];
} CreateRoomRequest;
```

### 3. **HTTP API (React ↔ Backend)**

**Request:**
```javascript
// Frontend
const response = await axios.post('http://localhost:5000/api/network/command', {
    command: 'CREATE_ROOM',
    account_id: 123,
    data: {
        room_name: 'Test Room',
        visibility: 0,
        mode: 0,
        max_players: 4,
        round_time: 30,
        wager_enabled: 0
    }
});
```

**Response:**
```json
{
    "success": true,
    "room": {
        "room_id": 123,
        "room_code": "ABC123",
        "host_id": 1,
        "created_at": 1702886400000
    }
}
```

---

## 🧪 TESTING GUIDE

### **Test 1: C Client (Raw Socket)**

```bash
cd Network/client
make clean && make
./build/client

# Chọn 1 (Create Room)
# Nhập: Test Room, 0, 0, 4, 30, 0
# Kết quả: ✅ Room created! Code: ABC123
```

### **Test 2: React UI (HTTP → Socket)**

```bash
# Start all services
cd Backend && python app/main.py &
cd Network/server && make && ./build/server &
cd Frontend && npm start

# Browser: http://localhost:3000/lobby
# Click "Create new room", điền form, submit
# Kết quả: Alert "Room created! Code: ABC123"
```

---

## ✅ STATUS

- ✅ C Client với RAW TCP Socket (đáp ứng yêu cầu môn học)
- ✅ React UI với HTTP (cho demo/production)
- ✅ Python Backend proxy: HTTP → RAW Socket → C Server
- ✅ C Server: Parse binary, route qua IPC
- ✅ 5 Host features hoàn chỉnh

**Ready for testing & submission! 🎉**
