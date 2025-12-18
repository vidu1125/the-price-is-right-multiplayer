# 🎮 HOST FEATURES - IMPLEMENTATION COMPLETE

## ✅ Summary

Đã implement đầy đủ 5 use cases của Host theo architecture mới:

1. **Create Room** - Tạo phòng chơi mới
2. **Set Rules** - Cài đặt luật chơi
3. **Kick Member** - Đuổi người chơi
4. **Delete Room** - Xóa phòng
5. **Start Game** - Bắt đầu trận đấu (với client-side countdown)

---

## 🏗️ Architecture Overview

```
┌────────────────────────────────────────────────────────┐
│                  CLIENT (Binary)                       │
└──────────────────────┬─────────────────────────────────┘
                       │ TCP Port 8888
┌──────────────────────▼─────────────────────────────────┐
│         C NETWORK LAYER (Transport Only)               │
│  - Parse binary packets                                │
│  - Session management (socket_fd → user_id)            │
│  - Forward to backend via IPC                          │
│                                                         │
│  Files:                                                 │
│  - main.c: select() loop                               │
│  - router.c: Route commands                            │
│  - backend_bridge.c: IPC client                        │
│  - session.c: Session CRUD                             │
│  - packet_handler.c: Send/recv                         │
└──────────────────────┬─────────────────────────────────┘
                       │ Unix Socket IPC
                       │ /tmp/tpir_backend.sock
┌──────────────────────▼─────────────────────────────────┐
│         PYTHON BACKEND (Business Logic)                │
│  - Input validation                                     │
│  - Business rules                                       │
│  - Database operations (SQLAlchemy)                     │
│  - Time sync                                            │
│                                                         │
│  Files:                                                 │
│  - services/host/room_service.py                       │
│  - services/host/rule_service.py                       │
│  - services/host/member_service.py                     │
│  - ipc/__init__.py: IPC server                         │
│  - utils/validation.py                                  │
│  - utils/time_sync.py                                   │
└──────────────────────┬─────────────────────────────────┘
                       │ SQLAlchemy ORM
┌──────────────────────▼─────────────────────────────────┐
│         PostgreSQL (Supabase)                          │
│  - rooms, room_members                                  │
│  - matches, match_players                               │
│  - accounts, profiles                                   │
└─────────────────────────────────────────────────────────┘
```

---

## 📁 Files Created/Modified

### Python Backend
```
Backend/app/
├── services/host/
│   ├── __init__.py                 ✅ NEW
│   ├── room_service.py             ✅ NEW (Create, Delete, Start Game)
│   ├── rule_service.py             ✅ NEW (Set Rules)
│   └── member_service.py           ✅ NEW (Kick Member)
├── utils/
│   ├── validation.py               ✅ NEW
│   └── time_sync.py                ✅ NEW
├── ipc/
│   └── __init__.py                 ✅ NEW (IPC Server)
└── main.py                         ✏️ MODIFIED (Start IPC)
```

### C Network Layer
```
Network/
├── protocol/
│   ├── commands.h                  ✅ NEW
│   └── payloads.h                  ✅ NEW
├── server/
│   ├── include/
│   │   ├── session.h               ✅ NEW
│   │   ├── backend_bridge.h        ✅ NEW
│   │   ├── packet_handler.h        ✅ NEW
│   │   └── router.h                ✅ NEW
│   ├── src/
│   │   ├── main.c                  ✅ NEW
│   │   ├── session.c               ✅ NEW
│   │   ├── backend_bridge.c        ✅ NEW
│   │   ├── packet_handler.c        ✅ NEW
│   │   └── router.c                ✅ NEW
│   └── Makefile                    ✅ NEW
└── HOST_FEATURES_README.md         ✅ NEW
```

---

## 🚀 Getting Started

### Step 1: Install Dependencies

**Python:**
```bash
cd Backend
pip install Flask Flask-CORS Flask-SQLAlchemy psycopg2-binary python-dotenv
```

**C (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install build-essential libcjson-dev
```

**C (macOS):**
```bash
brew install cjson
```

### Step 2: Start Python Backend

```bash
cd Backend
python app/main.py
```

**Expected output:**
```
🚀 Backend API starting on port 5000...
✅ Database connected
✅ Session manager initialized

🔌 Starting IPC server...
[IPC] Server listening on /tmp/tpir_backend.sock
 * Running on http://0.0.0.0:5000
```

### Step 3: Build C Server

```bash
cd Network/server
make clean
make
```

**Expected output:**
```
gcc -Wall -Wextra -I./include -I../protocol -c src/main.c -o build/main.o
gcc -Wall -Wextra -I./include -I../protocol -c src/router.c -o build/router.o
...
✅ Build complete: build/server
```

### Step 4: Run C Server

```bash
./build/server
```

**Expected output:**
```
═══════════════════════════════════════
  The Price Is Right - Socket Server
  Network Layer (C)
═══════════════════════════════════════

[Session] Manager initialized
[Init] Connecting to Python backend...
[Bridge] ✅ Connected to backend IPC at /tmp/tpir_backend.sock
✅ Listening on port 8888

Ready to accept connections...
```

---

## 🧪 Testing

### Test 1: Create Room (Manual)

**Using Python client:**
```python
import socket
import struct
import json

def send_create_room():
    sock = socket.socket()
    sock.connect(('localhost', 8888))
    
    # Header: magic=0x4347, version=1, command=0x0200
    # Payload: room name + settings
    room_name = b"My Test Room" + b'\x00' * (64 - 12)
    payload = room_name + struct.pack('BBHBB', 
        0,   # visibility: public
        0,   # mode: scoring
        4,   # max_players
        15,  # round_time (network byte order needs htons)
        0    # wager_enabled
    )
    
    # Fix: round_time needs to be network byte order
    payload = room_name + struct.pack('BBH', 0, 0, 4)
    payload += struct.pack('!H', 15)  # network byte order
    payload += struct.pack('B', 0)
    
    header = struct.pack('!HBBHHLL', 
        0x4347,         # magic
        0x01,           # version
        0,              # flags
        0x0200,         # command: CREATE_ROOM
        0,              # reserved
        0,              # seq_num
        len(payload)    # length
    )
    
    sock.sendall(header + payload)
    
    # Receive response
    resp_header = sock.recv(16)
    if len(resp_header) == 16:
        magic, ver, flags, code, _, _, length = struct.unpack('!HBBHHLL', resp_header)
        print(f"Response: code={code}, length={length}")
        
        if length > 0:
            resp_payload = sock.recv(length)
            if code == 202:  # RES_ROOM_CREATED
                room_id, code_str, host_id, timestamp = struct.unpack('!I11sIQ', resp_payload[:27])
                print(f"✅ Room created! ID={room_id}, Code={code_str.decode().strip()}")
    
    sock.close()

send_create_room()
```

---

## 📊 Protocol Examples

### Create Room Request
```
Header (16 bytes):
  Magic: 0x4347
  Version: 0x01
  Command: 0x0200 (CMD_CREATE_ROOM)
  Length: 73 bytes

Payload (73 bytes):
  room_name[64]: "My Test Room\0..."
  visibility: 0 (public)
  mode: 0 (scoring)
  max_players: 4
  round_time: 15 (network byte order)
  wager_enabled: 0
```

### Create Room Response
```
Header:
  Magic: 0x4347
  Command: 202 (RES_ROOM_CREATED)
  Length: 27 bytes

Payload:
  room_id: 123 (uint32_t)
  room_code: "ABC123\0..." (11 bytes)
  host_id: 1 (uint32_t)
  created_at: 1703001234567 (uint64_t timestamp ms)
```

---

## 🔍 Debugging

### Check IPC Connection
```bash
# Python backend should create socket file
ls -la /tmp/tpir_backend.sock

# Should show:
# srwxrwxrwx 1 user user 0 Dec 17 10:00 /tmp/tpir_backend.sock
```

### Monitor IPC Traffic
```bash
# Terminal 1: Backend
cd Backend
python app/main.py

# Terminal 2: C Server
cd Network/server
./build/server

# Terminal 3: Test client
python test_client.py

# Watch logs in Terminal 1 and 2
```

### Common Issues

**1. "Backend connection error"**
- Make sure Python backend is running FIRST
- Check `/tmp/tpir_backend.sock` exists

**2. "cJSON library not found"**
```bash
sudo apt-get install libcjson-dev
# or
brew install cjson
```

**3. "Session not found"**
- Session is created when client connects
- Make sure you're using the same socket connection

---

## 📝 Design Decisions

### Why IPC instead of direct database access in C?
✅ **Separation of Concerns**
- C handles transport only
- Python handles business logic
- Easy to test and maintain

✅ **Development Speed**
- Python for rapid prototyping
- C for performance-critical parts

✅ **Team Collaboration**
- Multiple developers can work on different layers
- No code conflicts

### Why Unix Socket for IPC?
✅ **Low Latency**: ~0.1ms (vs HTTP ~1-2ms)
✅ **Simple**: File-based, no network config
✅ **Secure**: Local-only, no network exposure

### Why Binary Protocol?
✅ **Efficiency**: Smaller packet size
✅ **Speed**: Fast parsing
✅ **Type Safety**: Fixed struct layout

---

## 🎯 Next Steps for Other Developers

### Adding a New Host Feature

**Example: Add "Transfer Host" feature**

1. **Define protocol** (`protocol/commands.h`):
```c
CMD_TRANSFER_HOST = 0x0208,
```

2. **Define payload** (`protocol/payloads.h`):
```c
typedef struct PACKED {
    uint32_t room_id;
    uint32_t new_host_id;
} TransferHostRequest;
```

3. **Add Python service** (`services/host/room_service.py`):
```python
def transfer_host(user_id, data):
    # Validate, update database, return response
    ...
```

4. **Add IPC route** (`ipc/__init__.py`):
```python
elif action == 'TRANSFER_HOST':
    from app.services.host.room_service import transfer_host
    return transfer_host(user_id, data)
```

5. **Add C router** (`server/src/router.c`):
```c
void route_transfer_host(int client_fd, TransferHostRequest* req) {
    // Build JSON, send to backend, handle response
    ...
}
```

6. **Add to router switch** (`server/src/router.c`):
```c
case CMD_TRANSFER_HOST:
    route_transfer_host(client_fd, (TransferHostRequest*)payload);
    break;
```

Done! ✅

---

## 📚 References

- [Application Layer Design Doc](../Docs/APPLICATION%20DESIGN.txt)
- [Game Rules](../rule.md)
- [Database Schema](../Database/init.sql)

---

## 👥 Team Members

**Host Features Team:**
- Your Name - Host use cases implementation
- [Add team members working on this branch]

**Other Teams:**
- Gameplay Team - Round 1/2/3 implementation
- Authentication Team - Login/Register
- Social Features Team - Friends, Chat

---

## ✅ Checklist

- [x] Protocol definitions
- [x] Python backend services
- [x] IPC server
- [x] C network layer
- [x] Session management
- [x] Create Room
- [x] Set Rules
- [x] Kick Member
- [x] Delete Room
- [x] Start Game (with non-blocking countdown)
- [x] Documentation
- [ ] Unit tests (Python)
- [ ] Integration tests
- [ ] Load testing

---

**Status:** ✅ Ready for merge to `main`
**Branch:** `feature/hostv2`
**Date:** December 17, 2025
