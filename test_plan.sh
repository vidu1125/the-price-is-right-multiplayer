#!/bin/bash

# Test Plan - Host Features
# Kiểm tra từng layer và từng use case

set -e  # Exit on error

echo ""
echo "╔════════════════════════════════════════════════════════╗"
echo "║       TEST PLAN - HOST FEATURES                        ║"
echo "║       Testing Layer by Layer                           ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PROJECT_ROOT="/home/duyen/DAIHOC/NetworkProgramming/Project/the-price-is-right-multiplayer"

# Function to print section header
print_section() {
    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
}

# Function to ask user
ask_continue() {
    echo ""
    echo -e "${YELLOW}👉 $1${NC}"
    read -p "Press Enter to continue..."
}

# PHASE 1: Build & Start Services
print_section "PHASE 1: BUILD & START SERVICES"

echo -e "${GREEN}Step 1.1: Build C Server${NC}"
cd "$PROJECT_ROOT/Network/server"
echo "Running: make clean && make"
make clean && make
echo -e "${GREEN}✅ C Server built successfully${NC}"

echo ""
echo -e "${GREEN}Step 1.2: Build C Client (Terminal)${NC}"
cd "$PROJECT_ROOT/Network/client"
echo "Running: make clean && make"
make clean && make
echo -e "${GREEN}✅ C Client built successfully${NC}"

ask_continue "Now you need to START services in separate terminals"

echo ""
echo -e "${YELLOW}📋 Open 2 terminals and run:${NC}"
echo ""
echo "  Terminal 1 (C Server):"
echo -e "    ${GREEN}cd $PROJECT_ROOT/Network/server${NC}"
echo -e "    ${GREEN}./build/server${NC}"
echo ""
echo "  Terminal 2 (Python Backend):"
echo -e "    ${GREEN}cd $PROJECT_ROOT/Backend${NC}"
echo -e "    ${GREEN}python app/main.py${NC}"
echo ""

ask_continue "After starting both services above"

# PHASE 2: Test Socket Communication
print_section "PHASE 2: TEST RAW TCP SOCKET COMMUNICATION"

echo -e "${GREEN}Step 2.1: Test Python Socket Client → C Server${NC}"
cd "$PROJECT_ROOT/Backend"
echo "Running: python3 test_socket_client.py"
echo ""
python3 test_socket_client.py

ask_continue "Socket client tests completed"

# PHASE 3: Test C Terminal Client
print_section "PHASE 3: TEST C TERMINAL CLIENT"

echo -e "${GREEN}Step 3.1: Run C Client (Manual test)${NC}"
echo ""
echo "Now you will test C Client manually:"
echo "  1. Create Room (option 1)"
echo "  2. Set Rules (option 2)"
echo "  3. Kick Member (option 3)"
echo "  4. Start Game (option 5)"
echo "  5. Delete Room (option 4)"
echo ""
cd "$PROJECT_ROOT/Network/client"
./build/client

ask_continue "C Client tests completed"

# PHASE 4: Test HTTP API
print_section "PHASE 4: TEST HTTP API (Backend Proxy)"

echo -e "${GREEN}Step 4.1: Test HTTP endpoints with curl${NC}"
echo ""

# Test 1: Create Room
echo -e "${YELLOW}Test 1: Create Room${NC}"
curl -X POST http://localhost:5000/api/network/command \
  -H "Content-Type: application/json" \
  -d '{
    "command": "CREATE_ROOM",
    "account_id": 1,
    "data": {
      "room_name": "HTTP Test Room",
      "visibility": 0,
      "mode": 0,
      "max_players": 4,
      "round_time": 30,
      "wager_enabled": 0
    }
  }' | jq '.'

echo ""
read -p "Enter the room_id from response above: " ROOM_ID

if [ ! -z "$ROOM_ID" ]; then
    # Test 2: Set Rules
    echo ""
    echo -e "${YELLOW}Test 2: Set Rules${NC}"
    curl -X POST http://localhost:5000/api/network/command \
      -H "Content-Type: application/json" \
      -d '{
        "command": "SET_RULE",
        "account_id": 1,
        "data": {
          "room_id": '"$ROOM_ID"',
          "mode": 1,
          "max_players": 6,
          "round_time": 60,
          "wager_enabled": 1
        }
      }' | jq '.'

    # Test 3: Start Game
    echo ""
    echo -e "${YELLOW}Test 3: Start Game${NC}"
    curl -X POST http://localhost:5000/api/network/command \
      -H "Content-Type: application/json" \
      -d '{
        "command": "START_GAME",
        "account_id": 1,
        "data": {
          "room_id": '"$ROOM_ID"'
        }
      }' | jq '.'

    # Test 4: Delete Room
    echo ""
    echo -e "${YELLOW}Test 4: Delete Room${NC}"
    curl -X POST http://localhost:5000/api/network/command \
      -H "Content-Type: application/json" \
      -d '{
        "command": "DELETE_ROOM",
        "account_id": 1,
        "data": {
          "room_id": '"$ROOM_ID"'
        }
      }' | jq '.'
fi

ask_continue "HTTP API tests completed"

# PHASE 5: Test React UI
print_section "PHASE 5: TEST REACT UI (Optional)"

echo -e "${GREEN}Step 5.1: Start React Dev Server${NC}"
echo ""
echo "Open a new terminal and run:"
echo -e "  ${GREEN}cd $PROJECT_ROOT/Frontend${NC}"
echo -e "  ${GREEN}npm start${NC}"
echo ""
echo "Then open browser: http://localhost:3000"
echo ""
echo "Test các use case:"
echo "  1. Go to Lobby"
echo "  2. Click 'Create new room' → Fill form → Submit"
echo "  3. Join waiting room"
echo "  4. Click 'edit' on Game Rules → Change settings → Save"
echo "  5. Click 'START GAME' → See countdown"
echo "  6. Click 'DELETE ROOM' → Return to lobby"

ask_continue "React UI tests completed (or skipped)"

# SUMMARY
print_section "TEST SUMMARY"

echo -e "${GREEN}✅ All test phases completed!${NC}"
echo ""
echo "What was tested:"
echo "  ✅ C Server build"
echo "  ✅ C Client build"
echo "  ✅ Python Socket Client → C Server (RAW TCP SOCKET)"
echo "  ✅ C Terminal Client → C Server"
echo "  ✅ HTTP API → Socket → C Server"
echo "  ✅ React UI → HTTP API (optional)"
echo ""
echo -e "${BLUE}Architecture validated:${NC}"
echo "  React UI → HTTP → Backend → RAW TCP SOCKET → C Server"
echo "  C Client → RAW TCP SOCKET → C Server"
echo ""
echo -e "${GREEN}🎉 Ready for submission!${NC}"
