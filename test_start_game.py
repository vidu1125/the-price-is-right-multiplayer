import socket
import struct
import json
import time

SERVER_IP = "127.0.0.1"
SERVER_PORT = 5500 # Will verify and update if needed

# Opcodes
CMD_LOGIN_REQ = 0x0100
CMD_REGISTER_REQ = 0x0101
CMD_CREATE_ROOM = 0x0200
CMD_START_GAME = 0x0300

# Responses
RES_LOGIN_OK = 0x00C9
RES_ROOM_CREATED = 0x00DC
RES_GAME_STARTED = 0x012D

def mk_header(opcode, length):
    # Header format: uint16_t opcode, uint16_t length (Big Endian usually for network?)
    # Wait, check protocol.h. Assume Little Endian based on typical convenience or Big Endian standard?
    # Usually network protocols are Big Endian (Network Byte Order).
    # usage in C: 
    # req->opcode = htons(opcode);
    # req->length = htonl(length); 
    # Let's check protocol definition. Assuming Big Endian (!)
    return struct.pack("!HI", opcode, length)

def read_header(sock):
    data = sock.recv(6) # 2 bytes opcode + 4 bytes length
    if len(data) < 6:
        return None
    opcode, length = struct.unpack("!HI", data)
    return opcode, length

def send_packet(sock, opcode, payload):
    if isinstance(payload, str):
        payload = payload.encode('utf-8')
    
    header = mk_header(opcode, len(payload))
    sock.sendall(header + payload)

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((SERVER_IP, SERVER_PORT))
        print(f"Connected to {SERVER_IP}:{SERVER_PORT}")

        # 1. Login
        # Use a likely existing test account or register one
        email = "handler_test@gmail.com"
        password = "password123"
        
        # Register first just in case
        print("Registering/Logging in...")
        reg_payload = json.dumps({
            "email": email, 
            "password": password, 
            "confirm": password,
            "name": "HandlerTester"
        })
        send_packet(s, CMD_REGISTER_REQ, reg_payload)
        
        # Read register/login response
        while True:
            resp = read_header(s)
            if not resp: break
            op, length = resp
            payload = s.recv(length)
            
            # 0x0100 Login Request? No, Server sends RES_...
            # If server sends opcodes back as responses or specific RES codes?
            # RES_SUCCESS = 0x00C8, RES_LOGIN_OK = 0x00C9
            print(f"Received OP: 0x{op:04X} Len: {length}")
            print(f"Payload: {payload.decode('utf-8', errors='ignore')}")
            
            if op == RES_LOGIN_OK or op == 0x00C8: # Success
                 break
            if op == 0x0190 or op == 0x0192: # Bad request / already exists -> Try login
                 login_payload = json.dumps({"email": email, "password": password})
                 send_packet(s, CMD_LOGIN_REQ, login_payload)
                 continue

        # 2. Create Room
        print("\nCreating Room...")
        # Struct: name(64), visibility(1), mode(1), max(1), time(1), wager(1), res(3)
        room_name = b"BackendTestRoom"
        room_name_padded = room_name + b'\0' * (64 - len(room_name))
        
        # visibility: 0 (Public), mode: 0, max: 4, time: 30, wager: 1
        create_payload = struct.pack("!64sBBBBB3s", 
                                     room_name_padded, 
                                     0, # Vis
                                     0, # Mode 
                                     4, # Max
                                     30,# Time
                                     1, # Wager
                                     b'\0\0\0')
        send_packet(s, CMD_CREATE_ROOM, create_payload)
        
        room_id = 0
        
        while True:
            resp = read_header(s)
            if not resp: break
            op, length = resp
            data = s.recv(length)
            print(f"Received OP: 0x{op:04X} Payload: {data.decode('utf-8', errors='ignore')}")
            
            if op == RES_ROOM_CREATED:
                # payload might be JSON with room_id, let's parse it
                try:
                    js = json.loads(data)
                    room_id = js.get("room_id")
                    print(f"Room Created ID: {room_id}")
                    break
                except:
                    print("Could not parse room creation response")
                    return

        if room_id == 0:
            print("Failed to get Room ID")
            return

        # 3. Start Game
        print(f"\nStarting Game for Room {room_id}...")
        # Payload: room_id (uint32)
        start_payload = struct.pack("!I", room_id)
        send_packet(s, CMD_START_GAME, start_payload)
        
        # 4. Wait for Game Start Response
        while True:
            resp = read_header(s)
            if not resp: break
            op, length = resp
            data = s.recv(length)
            print(f"Received OP: 0x{op:04X} Len: {length}")
            if length > 0:
                 print(f"Payload: {data.decode('utf-8', errors='ignore')}")
            
            if op == RES_GAME_STARTED:
                print("SUCCESS: Game Started!")
                break
            
            # Check for broadcast notification OP_S2C_ROUND1_START?
            # NTF_GAME_START = 0x02C4
            if op == 0x02C4: 
                print("SUCCESS: Received Game Start Notification!")
                break
                
        print("Test Complete.")

    except Exception as e:
        print(f"Error: {e}")
    finally:
        s.close()

if __name__ == "__main__":
    main()
