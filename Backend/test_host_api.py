#!/usr/bin/env python3
"""
Test script for Host API endpoints
"""
import requests
import json
import sys

BASE_URL = "http://localhost:5000"
ACCOUNT_ID = "1"  # Test account

def print_section(title):
    print("\n" + "=" * 60)
    print(f"  {title}")
    print("=" * 60)

def print_result(status_code, response):
    status_icon = "✅" if 200 <= status_code < 300 else "❌"
    print(f"{status_icon} Status: {status_code}")
    try:
        data = response.json()
        print(f"Response:\n{json.dumps(data, indent=2)}")
        return data
    except:
        print(f"Response (raw): {response.text[:200]}")
        return None

def test_1_create_room():
    print_section("TEST 1: CREATE ROOM")
    
    data = {
        "name": "Host Test Room",
        "visibility": "public",
        "mode": "scoring",
        "max_players": 6,
        "round_time": 15,
        "advanced": {"wager": True}
    }
    
    try:
        resp = requests.post(
            f"{BASE_URL}/api/room/create",
            json=data,
            headers={"X-Account-ID": ACCOUNT_ID},
            timeout=5
        )
        result = print_result(resp.status_code, resp)
        
        if result and result.get('success'):
            room_id = result.get('room_id')
            room_code = result.get('room', {}).get('code')
            print(f"\n🎉 Room created: ID={room_id}, Code={room_code}")
            return room_id
        else:
            print("\n❌ Failed to create room")
            return None
            
    except requests.exceptions.ConnectionError:
        print("❌ Connection Error: Backend not running on port 5000")
        print("   Run: docker-compose up backend")
        return None
    except Exception as e:
        print(f"❌ Error: {e}")
        return None

def test_2_update_rules(room_id):
    print_section("TEST 2: UPDATE ROOM RULES")
    
    if not room_id:
        print("⏭️  Skipped (no room_id)")
        return
    
    data = {
        "room_id": room_id,
        "rules": {
            "mode": "elimination",
            "max_players": 4,
            "round_time": 10,
            "advanced": {"wager": False}
        }
    }
    
    try:
        resp = requests.put(
            f"{BASE_URL}/api/room/rules",
            json=data,
            headers={"X-Account-ID": ACCOUNT_ID},
            timeout=5
        )
        result = print_result(resp.status_code, resp)
        
        if result and result.get('success'):
            print("\n✅ Rules updated successfully")
        else:
            print("\n❌ Failed to update rules")
            
    except Exception as e:
        print(f"❌ Error: {e}")

def test_3_get_room_details(room_id):
    print_section("TEST 3: GET ROOM DETAILS")
    
    if not room_id:
        print("⏭️  Skipped (no room_id)")
        return
    
    try:
        resp = requests.get(
            f"{BASE_URL}/api/room/{room_id}",
            headers={"X-Account-ID": ACCOUNT_ID},
            timeout=5
        )
        result = print_result(resp.status_code, resp)
        
        if result and result.get('success'):
            room = result.get('room', {})
            print(f"\n📋 Room Details:")
            print(f"   Name: {room.get('name')}")
            print(f"   Code: {room.get('code')}")
            print(f"   Mode: {room.get('mode')}")
            print(f"   Max Players: {room.get('max_players')}")
            print(f"   Members: {len(room.get('members', []))}")
        
    except Exception as e:
        print(f"❌ Error: {e}")

def test_4_close_room(room_id):
    print_section("TEST 4: CLOSE ROOM")
    
    if not room_id:
        print("⏭️  Skipped (no room_id)")
        return
    
    try:
        resp = requests.delete(
            f"{BASE_URL}/api/room/{room_id}",
            headers={"X-Account-ID": ACCOUNT_ID},
            timeout=5
        )
        result = print_result(resp.status_code, resp)
        
        if result and result.get('success'):
            print("\n✅ Room closed successfully")
        else:
            print("\n❌ Failed to close room")
            
    except Exception as e:
        print(f"❌ Error: {e}")

def main():
    print("\n" + "🚀" * 30)
    print("  BACKEND HOST API TEST SUITE")
    print("🚀" * 30)
    
    # Check backend health
    try:
        resp = requests.get(f"{BASE_URL}/health/db", timeout=2)
        if resp.status_code == 200:
            print("✅ Backend is running")
        else:
            print("⚠️  Backend responding but health check failed")
    except:
        print("❌ Backend not accessible at http://localhost:5000")
        print("   Please run: docker-compose up backend")
        sys.exit(1)
    
    # Run tests
    room_id = test_1_create_room()
    test_2_update_rules(room_id)
    test_3_get_room_details(room_id)
    test_4_close_room(room_id)
    
    print("\n" + "=" * 60)
    print("  TEST COMPLETED")
    print("=" * 60 + "\n")

if __name__ == "__main__":
    main()
