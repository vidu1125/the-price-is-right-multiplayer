#!/usr/bin/env python3
"""
Quick Health Check for Game Server
Tests: Port listening, Basic connection, Protocol handshake
"""

import socket
import struct
import sys
import subprocess
import time

HOST = 'localhost'
PORT = 5500
MAGIC_NUMBER = 0x4347
CMD_HEARTBEAT = 0x0001

def print_status(check_name, passed, message=""):
    status = "✅ PASS" if passed else "❌ FAIL"
    print(f"{status}  {check_name}")
    if message:
        print(f"     {message}")

def check_port_listening():
    """Check if port 5500 is listening"""
    try:
        result = subprocess.run(
            ['lsof', '-i', f':{PORT}'],
            capture_output=True,
            text=True,
            timeout=2
        )
        
        if result.returncode == 0 and 'LISTEN' in result.stdout:
            return True, f"Port {PORT} is listening"
        else:
            return False, f"Port {PORT} is not listening"
    except:
        # Fallback to netstat
        try:
            result = subprocess.run(
                ['netstat', '-an'],
                capture_output=True,
                text=True,
                timeout=2
            )
            if f'.{PORT}' in result.stdout or f':{PORT}' in result.stdout:
                return True, f"Port {PORT} appears to be open"
            else:
                return False, f"Port {PORT} not found"
        except:
            return None, "Unable to check port status"

def check_basic_connection():
    """Check if we can connect to the server"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(3)
        sock.connect((HOST, PORT))
        sock.close()
        return True, f"Successfully connected to {HOST}:{PORT}"
    except ConnectionRefusedError:
        return False, "Connection refused - Server not running"
    except socket.timeout:
        return False, "Connection timeout"
    except Exception as e:
        return False, f"Connection error: {e}"

def check_protocol_handshake():
    """Check if server responds to protocol messages"""
    sock = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(3)
        sock.connect((HOST, PORT))
        
        # Send heartbeat
        header = struct.pack(
            '>HBBHHII',
            MAGIC_NUMBER,  # magic
            0x01,          # version
            0x00,          # flags
            CMD_HEARTBEAT, # command
            0x0000,        # reserved
            1,             # seq_num
            0              # length
        )
        sock.sendall(header)
        
        # ✅ CHỜ SERVER GỬI RESPONSE XONG
        response = sock.recv(1024)
        
        if len(response) >= 6:
            magic = struct.unpack('>H', response[:2])[0]
            if magic == MAGIC_NUMBER:
                result = True, "Server responds with valid protocol"
            else:
                result = False, f"Invalid magic number: 0x{magic:04X}"
        else:
            result = False, f"Response too short: {len(response)} bytes"
        
        # ✅ ĐỢI MỘT CHÚT ĐỂ SERVER HOÀN TẤT
        time.sleep(0.2)
        
        return result
            
    except socket.timeout:
        return False, "Server did not respond (timeout)"
    except Exception as e:
        return False, f"Protocol error: {e}"
    finally:
        # ✅ GRACEFUL SHUTDOWN
        if sock:
            try:
                # Shutdown write first
                sock.shutdown(socket.SHUT_WR)
                # Wait a bit for server to finish
                time.sleep(0.1)
                # Then close
                sock.close()
            except:
                # Ignore errors during cleanup
                pass

def check_database():
    """Check if database file exists"""
    import os
    if os.path.exists('game_server.db'):
        size = os.path.getsize('game_server.db')
        return True, f"Database exists ({size} bytes)"
    else:
        return False, "Database file not found"

def check_server_process():
    """Check if server process is running"""
    try:
        result = subprocess.run(
            ['ps', 'aux'],
            capture_output=True,
            text=True,
            timeout=2
        )
        
        if 'game_server' in result.stdout:
            # Extract PID
            lines = [l for l in result.stdout.split('\n') if 'game_server' in l and 'grep' not in l]
            if lines:
                pid = lines[0].split()[1]
                return True, f"Server process running (PID: {pid})"
        
        return False, "Server process not found"
    except:
        return None, "Unable to check process"

def main():
    print()
    print("═" * 70)
    print("  GAME SERVER HEALTH CHECK".center(70))
    print("═" * 70)
    print()
    
    checks = [
        ("Server Process", check_server_process),
        ("Database File", check_database),
        ("Port Listening", check_port_listening),
        ("TCP Connection", check_basic_connection),
        ("Protocol Handshake", check_protocol_handshake),
    ]
    
    results = []
    
    for name, check_func in checks:
        passed, message = check_func()
        print_status(name, passed, message)
        results.append(passed)
        time.sleep(0.3)  # ✅ Tăng delay giữa các test
    
    print()
    print("═" * 70)
    
    passed_count = sum(1 for r in results if r is True)
    total_count = len([r for r in results if r is not None])
    
    if passed_count == total_count:
        print("🎉 ALL CHECKS PASSED - Server is healthy!")
        print("═" * 70)
        print()
        print("✨ Your server is ready to accept connections!")
        print()
        print("Try running:")
        print("  python3 test_suite.py              # Run full test suite")
        print("  python3 test_suite.py interactive  # Interactive client")
        print()
        return 0
    else:
        print(f"⚠️  {passed_count}/{total_count} checks passed")
        print("═" * 70)
        print()
        print("Troubleshooting:")
        
        if not results[0]:  # Process
            print("  • Server not running. Start it with: make run")
        if not results[2]:  # Port
            print("  • Port 5500 not listening. Check for errors in server logs")
        if not results[3]:  # Connection
            print("  • Can't connect. Check firewall settings")
        if not results[4]:  # Protocol
            print("  • Protocol error. Check server implementation")
        
        print()
        return 1

if __name__ == '__main__':
    sys.exit(main())