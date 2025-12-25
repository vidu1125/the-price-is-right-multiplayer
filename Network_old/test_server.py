import socket
import struct
import time
import sys
import random
from typing import Optional, Tuple

# =============================================================================
# PROTOCOL CONSTANTS
# =============================================================================

MAGIC_NUMBER = 0x4347
PROTOCOL_VERSION = 0x01

# Commands
CMD_HEARTBEAT = 0x0001
CMD_LOGIN_REQ = 0x0100
CMD_REGISTER_REQ = 0x0101
CMD_LOGOUT_REQ = 0x0102
CMD_CREATE_ROOM = 0x0200
CMD_JOIN_ROOM = 0x0201
CMD_LEAVE_ROOM = 0x0202
CMD_READY = 0x0203
CMD_KICK = 0x0204
CMD_START_GAME = 0x0300
CMD_ANSWER_QUIZ = 0x0301
CMD_BID = 0x0302
CMD_SPIN = 0x0303
CMD_FORFEIT = 0x0304
CMD_CHAT = 0x0400
CMD_HIST = 0x0500
CMD_LEAD = 0x0501

# Responses
RES_SUCCESS = 200
RES_LOGIN_OK = 201
RES_HEARTBEAT_OK = 210
ERR_BAD_REQUEST = 400
ERR_NOT_LOGGED_IN = 401
ERR_INVALID_USERNAME = 402

# =============================================================================
# HELPER FUNCTIONS
# =============================================================================

def create_header(command: int, payload_length: int, seq_num: int = 1) -> bytes:
    """Create protocol header"""
    return struct.pack(
        '>HBBHHII',
        MAGIC_NUMBER,      # magic
        PROTOCOL_VERSION,  # version
        0x00,              # flags
        command,           # command
        0x0000,            # reserved
        seq_num,           # sequence number
        payload_length     # payload length
    )

def parse_header(data: bytes) -> Optional[dict]:
    """Parse response header"""
    if len(data) < 16:
        return None
    
    magic, version, flags, command, reserved, seq_num, length = struct.unpack(
        '>HBBHHII', data[:16]
    )
    
    return {
        'magic': magic,
        'version': version,
        'flags': flags,
        'command': command,
        'reserved': reserved,
        'seq_num': seq_num,
        'length': length
    }

def print_separator(char='=', length=70):
    """Print separator line"""
    print(char * length)

def print_success(msg: str):
    """Print success message"""
    print(f"✅ {msg}")

def print_error(msg: str):
    """Print error message"""
    print(f"❌ {msg}")

def print_info(msg: str):
    """Print info message"""
    print(f"ℹ️  {msg}")

# =============================================================================
# CLIENT CLASS
# =============================================================================

class GameClient:
    """Game client for testing"""
    
    def __init__(self, host='localhost', port=5500, timeout=5):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.sock: Optional[socket.socket] = None
        self.seq_num = 1
        self.user_id = 0
        self.session_id = 0
        self.balance = 0
        self.username = ""
        
    def connect(self) -> bool:
        """Connect to server"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(self.timeout)
            self.sock.connect((self.host, self.port))
            print_success(f"Connected to {self.host}:{self.port}")
            return True
        except ConnectionRefusedError:
            print_error("Connection refused - Is server running?")
            return False
        except Exception as e:
            print_error(f"Connection error: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from server"""
        if self.sock:
            self.sock.close()
            self.sock = None
            print_info("Disconnected")
    
    def send_message(self, command: int, payload: bytes = b"") -> bool:
        """Send message to server"""
        try:
            header = create_header(command, len(payload), self.seq_num)
            self.sock.sendall(header + payload)
            self.seq_num += 1
            return True
        except Exception as e:
            print_error(f"Send error: {e}")
            return False
    
    def receive_message(self) -> Optional[Tuple[dict, bytes]]:
        """Receive message from server"""
        try:
            # Receive header (16 bytes)
            header_data = self.sock.recv(16)
            if len(header_data) < 16:
                return None
            
            header = parse_header(header_data)
            if not header:
                return None
            
            # Receive payload
            payload = b""
            if header['length'] > 0:
                payload = self.sock.recv(header['length'])
            
            return header, payload
        except socket.timeout:
            print_error("Receive timeout")
            return None
        except Exception as e:
            print_error(f"Receive error: {e}")
            return None
    
    def heartbeat(self) -> bool:
        """Send heartbeat"""
        print_info("Sending heartbeat...")
        if not self.send_message(CMD_HEARTBEAT):
            return False
        
        response = self.receive_message()
        if not response:
            return False
        
        header, _ = response
        if header['command'] == RES_HEARTBEAT_OK:
            print_success("Heartbeat OK")
            return True
        else:
            print_error(f"Unexpected response: {header['command']}")
            return False
    
    def register(self, username: str, password: str) -> bool:
        """Register new user"""
        print_info(f"Registering user: {username}")
        
        # Create payload
        username_bytes = username.encode('utf-8')[:32].ljust(32, b'\x00')
        password_bytes = password.encode('utf-8')[:64].ljust(64, b'\x00')
        payload = username_bytes + password_bytes
        
        if not self.send_message(CMD_REGISTER_REQ, payload):
            return False
        
        response = self.receive_message()
        if not response:
            return False
        
        header, payload = response
        if header['command'] == RES_SUCCESS:
            print_success("Registration successful")
            return True
        elif header['command'] == ERR_BAD_REQUEST:
            print_error("Username already exists or invalid")
            return False
        else:
            print_error(f"Unexpected response: {header['command']}")
            return False
    
    def login(self, username: str, password: str) -> bool:
        """Login user"""
        print_info(f"Logging in as: {username}")
        
        # Create payload
        username_bytes = username.encode('utf-8')[:32].ljust(32, b'\x00')
        password_bytes = password.encode('utf-8')[:64].ljust(64, b'\x00')
        payload = username_bytes + password_bytes
        
        if not self.send_message(CMD_LOGIN_REQ, payload):
            return False
        
        response = self.receive_message()
        if not response:
            return False
        
        header, payload = response
        if header['command'] == RES_LOGIN_OK:
            # Parse login response
            if len(payload) >= 12:
                self.user_id, self.session_id, self.balance = struct.unpack(
                    '>III', payload[:12]
                )
                self.username = username
                print_success(f"Login successful!")
                print_info(f"  User ID:    {self.user_id}")
                print_info(f"  Session ID: {self.session_id}")
                print_info(f"  Balance:    {self.balance}")
                return True
        elif header['command'] == ERR_NOT_LOGGED_IN:
            print_error("Invalid credentials")
        else:
            print_error(f"Unexpected response: {header['command']}")
        
        return False
    
    def logout(self) -> bool:
        """Logout user"""
        print_info("Logging out...")
        
        if not self.send_message(CMD_LOGOUT_REQ):
            return False
        
        response = self.receive_message()
        if not response:
            return False
        
        header, _ = response
        if header['command'] == RES_SUCCESS:
            print_success("Logout successful")
            self.user_id = 0
            self.session_id = 0
            self.balance = 0
            self.username = ""
            return True
        else:
            print_error(f"Logout failed: {header['command']}")
            return False
    
    def create_room(self, room_name: str, max_players: int = 4) -> bool:
        """Create game room"""
        print_info(f"Creating room: {room_name}")
        
        # Create payload (room name + max players)
        room_name_bytes = room_name.encode('utf-8')[:32].ljust(32, b'\x00')
        payload = room_name_bytes + struct.pack('>I', max_players)
        
        if not self.send_message(CMD_CREATE_ROOM, payload):
            return False
        
        response = self.receive_message()
        if not response:
            return False
        
        header, payload = response
        if header['command'] == RES_SUCCESS:
            print_success(f"Room created: {room_name}")
            return True
        else:
            print_error(f"Create room failed: {header['command']}")
            return False
    
    def join_room(self, room_id: int) -> bool:
        """Join game room"""
        print_info(f"Joining room: {room_id}")
        
        payload = struct.pack('>I', room_id)
        
        if not self.send_message(CMD_JOIN_ROOM, payload):
            return False
        
        response = self.receive_message()
        if not response:
            return False
        
        header, _ = response
        if header['command'] == RES_SUCCESS:
            print_success(f"Joined room {room_id}")
            return True
        else:
            print_error(f"Join room failed: {header['command']}")
            return False

# =============================================================================
# TEST FUNCTIONS
# =============================================================================

def test_connection():
    """Test basic connection"""
    print_separator()
    print("TEST 1: CONNECTION")
    print_separator()
    
    client = GameClient()
    if not client.connect():
        return False
    
    client.disconnect()
    return True

def test_heartbeat():
    """Test heartbeat"""
    print_separator()
    print("TEST 2: HEARTBEAT")
    print_separator()
    
    client = GameClient()
    if not client.connect():
        return False
    
    result = client.heartbeat()
    client.disconnect()
    return result

def test_register():
    """Test user registration"""
    print_separator()
    print("TEST 3: REGISTER")
    print_separator()
    
    username = f"testuser{random.randint(1000, 9999)}"
    password = "testpass123"
    
    client = GameClient()
    if not client.connect():
        return False
    
    result = client.register(username, password)
    client.disconnect()
    return result

def test_login():
    """Test user login"""
    print_separator()
    print("TEST 4: LOGIN")
    print_separator()
    
    # First register a user
    username = f"logintest{random.randint(1000, 9999)}"
    password = "testpass123"
    
    client = GameClient()
    if not client.connect():
        return False
    
    # Register
    if not client.register(username, password):
        client.disconnect()
        return False
    
    client.disconnect()
    time.sleep(0.5)
    
    # Now login
    if not client.connect():
        return False
    
    result = client.login(username, password)
    client.disconnect()
    return result

def test_logout():
    """Test user logout"""
    print_separator()
    print("TEST 5: LOGOUT")
    print_separator()
    
    username = f"logouttest{random.randint(1000, 9999)}"
    password = "testpass123"
    
    client = GameClient()
    if not client.connect():
        return False
    
    # Register and login
    if not client.register(username, password):
        client.disconnect()
        return False
    
    client.disconnect()
    time.sleep(0.5)
    
    if not client.connect():
        return False
    
    if not client.login(username, password):
        client.disconnect()
        return False
    
    # Logout
    result = client.logout()
    client.disconnect()
    return result

def test_multiple_clients():
    """Test multiple simultaneous clients"""
    print_separator()
    print("TEST 6: MULTIPLE CLIENTS")
    print_separator()
    
    clients = []
    num_clients = 3
    
    # Create and connect multiple clients
    for i in range(num_clients):
        username = f"multiclient{i}_{random.randint(100, 999)}"
        password = "testpass123"
        
        client = GameClient()
        if not client.connect():
            # Cleanup
            for c in clients:
                c.disconnect()
            return False
        
        if not client.register(username, password):
            client.disconnect()
            for c in clients:
                c.disconnect()
            return False
        
        client.disconnect()
        time.sleep(0.3)
        
        if not client.connect():
            for c in clients:
                c.disconnect()
            return False
        
        if not client.login(username, password):
            client.disconnect()
            for c in clients:
                c.disconnect()
            return False
        
        clients.append(client)
        print_success(f"Client {i+1} connected and logged in")
    
    # Send heartbeat from all clients
    print_info("Sending heartbeats from all clients...")
    for i, client in enumerate(clients):
        if not client.heartbeat():
            for c in clients:
                c.disconnect()
            return False
    
    # Cleanup
    for client in clients:
        client.disconnect()
    
    print_success(f"All {num_clients} clients handled successfully")
    return True

def test_invalid_login():
    """Test login with invalid credentials"""
    print_separator()
    print("TEST 7: INVALID LOGIN")
    print_separator()
    
    client = GameClient()
    if not client.connect():
        return False
    
    print_info("Attempting login with invalid credentials...")
    result = client.login("nonexistent_user", "wrong_password")
    
    client.disconnect()
    
    # This should fail - so we return True if login failed
    if not result:
        print_success("Invalid login correctly rejected")
        return True
    else:
        print_error("Invalid login should have been rejected!")
        return False

def test_room_creation():
    """Test room creation"""
    print_separator()
    print("TEST 8: ROOM CREATION")
    print_separator()
    
    username = f"roomcreator{random.randint(1000, 9999)}"
    password = "testpass123"
    
    client = GameClient()
    if not client.connect():
        return False
    
    # Register and login
    if not client.register(username, password):
        client.disconnect()
        return False
    
    client.disconnect()
    time.sleep(0.5)
    
    if not client.connect():
        return False
    
    if not client.login(username, password):
        client.disconnect()
        return False
    
    # Create room
    room_name = f"TestRoom{random.randint(100, 999)}"
    result = client.create_room(room_name, 4)
    
    client.disconnect()
    return result

# =============================================================================
# STRESS TEST
# =============================================================================

def stress_test(duration_seconds: int = 30):
    """Stress test with rapid connections"""
    print_separator()
    print(f"STRESS TEST: {duration_seconds} SECONDS")
    print_separator()
    
    start_time = time.time()
    connections = 0
    errors = 0
    
    print_info("Starting stress test...")
    
    while time.time() - start_time < duration_seconds:
        client = GameClient(timeout=2)
        
        if client.connect():
            connections += 1
            client.heartbeat()
            client.disconnect()
        else:
            errors += 1
        
        time.sleep(0.1)  # 10 connections per second
        
        if connections % 10 == 0:
            elapsed = time.time() - start_time
            print(f"  {connections} connections, {errors} errors, {elapsed:.1f}s elapsed")
    
    print_separator()
    print_success(f"Stress test complete!")
    print_info(f"  Total connections: {connections}")
    print_info(f"  Total errors: {errors}")
    print_info(f"  Success rate: {connections/(connections+errors)*100:.1f}%")
    
    return errors == 0

# =============================================================================
# MAIN TEST SUITE
# =============================================================================

def run_all_tests():
    """Run all tests"""
    print("\n")
    print("╔" + "═" * 68 + "╗")
    print("║" + "  GAME SERVER COMPREHENSIVE TEST SUITE".center(68) + "║")
    print("╚" + "═" * 68 + "╝")
    print("\n")
    
    tests = [
        ("Connection", test_connection),
        ("Heartbeat", test_heartbeat),
        ("Register", test_register),
        ("Login", test_login),
        ("Logout", test_logout),
        ("Multiple Clients", test_multiple_clients),
        ("Invalid Login", test_invalid_login),
        ("Room Creation", test_room_creation),
    ]
    
    results = []
    
    for name, test_func in tests:
        try:
            result = test_func()
            results.append((name, result))
            time.sleep(0.5)  # Pause between tests
        except Exception as e:
            print_error(f"Test '{name}' crashed: {e}")
            results.append((name, False))
    
    # Ask for stress test
    print_separator()
    response = input("\n⚡ Run stress test? (takes 30 seconds) [y/N]: ")
    if response.lower() == 'y':
        results.append(("Stress Test", stress_test(30)))
    
    # Summary
    print("\n")
    print_separator()
    print("TEST SUMMARY")
    print_separator()
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for name, result in results:
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"{status}  {name}")
    
    print_separator()
    print(f"Result: {passed}/{total} tests passed")
    print_separator()
    
    if passed == total:
        print("🎉 ALL TESTS PASSED!")
        return 0
    else:
        print("⚠️  SOME TESTS FAILED")
        return 1

# =============================================================================
# INTERACTIVE CLIENT
# =============================================================================

def interactive_client():
    """Interactive client for manual testing"""
    print("\n")
    print("╔" + "═" * 68 + "╗")
    print("║" + "  INTERACTIVE GAME CLIENT".center(68) + "║")
    print("╚" + "═" * 68 + "╝")
    print("\n")
    
    client = GameClient()
    
    if not client.connect():
        return
    
    while True:
        print("\n" + "─" * 70)
        print("Commands:")
        print("  1. Heartbeat")
        print("  2. Register")
        print("  3. Login")
        print("  4. Logout")
        print("  5. Create Room")
        print("  6. Join Room")
        print("  0. Quit")
        print("─" * 70)
        
        choice = input("Enter command: ").strip()
        
        if choice == '0':
            break
        elif choice == '1':
            client.heartbeat()
        elif choice == '2':
            username = input("Username: ").strip()
            password = input("Password: ").strip()
            client.register(username, password)
        elif choice == '3':
            username = input("Username: ").strip()
            password = input("Password: ").strip()
            client.login(username, password)
        elif choice == '4':
            client.logout()
        elif choice == '5':
            room_name = input("Room name: ").strip()
            max_players = int(input("Max players (2-4): ").strip() or "4")
            client.create_room(room_name, max_players)
        elif choice == '6':
            room_id = int(input("Room ID: ").strip())
            client.join_room(room_id)
        else:
            print_error("Invalid command")
    
    client.disconnect()
    print_info("Goodbye!")

# =============================================================================
# MAIN
# =============================================================================

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'interactive':
        interactive_client()
    else:
        sys.exit(run_all_tests())