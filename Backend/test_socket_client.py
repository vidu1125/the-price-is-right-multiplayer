#!/usr/bin/env python3
"""
Test Script - Socket Client to C Server
Tests RAW TCP SOCKET communication with binary protocol

Run this AFTER starting C Server:
    cd Network/server && make && ./build/server
"""

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from app.services.socket_client import get_socket_client
import logging

# Setup logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s [%(levelname)s] %(message)s'
)

def test_create_room():
    """Test 1: Create Room"""
    print("\n" + "="*60)
    print("TEST 1: CREATE ROOM")
    print("="*60)
    
    client = get_socket_client()
    
    result = client.create_room(
        room_name="Test Room",
        visibility=0,      # public
        mode=0,            # scoring
        max_players=4,
        round_time=30,
        wager_enabled=0
    )
    
    print("\n✅ Result:", result)
    
    if result.get('success'):
        print(f"   Room ID: {result['room_id']}")
        print(f"   Room Code: {result['room_code']}")
        print(f"   Host ID: {result['host_id']}")
        return result['room_id']
    else:
        print(f"   ❌ Error: {result.get('error')}")
        return None


def test_set_rule(room_id):
    """Test 2: Set Rules"""
    print("\n" + "="*60)
    print("TEST 2: SET RULES")
    print("="*60)
    
    if not room_id:
        print("⚠️  Skipping (no room_id)")
        return
    
    client = get_socket_client()
    
    result = client.set_rule(
        room_id=room_id,
        mode=1,            # elimination
        max_players=6,
        round_time=60,
        wager_enabled=1
    )
    
    print("\n✅ Result:", result)


def test_kick_member(room_id):
    """Test 3: Kick Member"""
    print("\n" + "="*60)
    print("TEST 3: KICK MEMBER")
    print("="*60)
    
    if not room_id:
        print("⚠️  Skipping (no room_id)")
        return
    
    client = get_socket_client()
    
    # Try to kick user_id 999 (probably doesn't exist, will fail)
    result = client.kick_member(
        room_id=room_id,
        target_user_id=999
    )
    
    print("\n✅ Result:", result)


def test_start_game(room_id):
    """Test 4: Start Game"""
    print("\n" + "="*60)
    print("TEST 4: START GAME")
    print("="*60)
    
    if not room_id:
        print("⚠️  Skipping (no room_id)")
        return
    
    client = get_socket_client()
    
    result = client.start_game(room_id=room_id)
    
    print("\n✅ Result:", result)
    
    if result.get('success'):
        print(f"   Match ID: {result['match_id']}")
        print(f"   Countdown: {result['countdown_ms']} ms")
        print(f"   Server Time: {result['server_timestamp_ms']} ms")
        print(f"   Game Start: {result['game_start_timestamp_ms']} ms")


def test_delete_room(room_id):
    """Test 5: Delete Room"""
    print("\n" + "="*60)
    print("TEST 5: DELETE ROOM")
    print("="*60)
    
    if not room_id:
        print("⚠️  Skipping (no room_id)")
        return
    
    client = get_socket_client()
    
    result = client.delete_room(room_id=room_id)
    
    print("\n✅ Result:", result)


def main():
    print("\n")
    print("╔════════════════════════════════════════════════════════╗")
    print("║  SOCKET CLIENT TEST - RAW TCP SOCKET TO C SERVER      ║")
    print("╚════════════════════════════════════════════════════════╝")
    
    print("\n⚠️  Prerequisites:")
    print("   1. C Server must be running: cd Network/server && ./build/server")
    print("   2. Python Backend IPC server: cd Backend && python app/main.py")
    
    input("\n👉 Press Enter to start tests...")
    
    # Run tests sequentially
    room_id = test_create_room()
    
    if room_id:
        test_set_rule(room_id)
        test_kick_member(room_id)
        test_start_game(room_id)
        test_delete_room(room_id)
    
    print("\n" + "="*60)
    print("✅ ALL TESTS COMPLETED")
    print("="*60)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n👋 Tests interrupted by user")
    except Exception as e:
        print(f"\n\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
