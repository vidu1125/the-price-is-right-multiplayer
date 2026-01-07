# The Price Is Right - Multiplayer Game

A real-time multiplayer price guessing game built with a **C/C++ Game Server**, **React Frontend**, and **Supabase Database**.

## 🏗 System Architecture

This project uses a layered architecture to combine low-level performance with modern web technologies:

```mermaid
graph TD
    Client[React Frontend] <-->|WebSocket| Bridge[WS Bridge (Node.js)]
    Bridge <-->|TCP Socket| Server[C Game Server]
    Server <-->|REST API| DB[(Supabase PostgreSQL)]
```

| Component | Technology | Port | Description |
|-----------|------------|------|-------------|
| **Frontend** | React.js | 3000 | Interactive UI for players. |
| **WS Bridge** | Node.js | 8080 | Translates WebSocket (Web) to TCP (Server). |
| **Network** | C/C++ | 5500 | Core game engine, state management, and business logic. |
| **Database** | PostgreSQL | 5432 | Data persistence (Users, Rooms, Matches) via Supabase. |

---

## 🚀 Quick Start

### Prerequisites
*   **Docker** & **Docker Compose** installed.
*   **Supabase** project (or a PostgreSQL instance with REST API enabled).

### 1. Configuration
Create a `.env` file in the root directory (based on `.env.example` if available):

```env
# Supabase Configuration
SUPABASE_URL=https://your-project.supabase.co
SUPABASE_KEY=your-anon-key

# Service Ports
SOCKET_SERVER_PORT=5500
WS_BRIDGE_PORT=8080
FRONTEND_PORT=3000
```

### 2. Run with Docker
Start the entire stack with one command:

```bash
docker-compose up --build
```

Wait a few minutes. Access the game at: **http://localhost:3000**

---

## 📂 Project Structure

```
the-price-is-right-multiplayer/
├── Frontend/           # React Application
│   ├── src/
│   │   ├── components/ # UI Components (Lobby, Room, Game)
│   │   ├── services/   # API & Logic (hostService.js)
│   │   └── network/    # WebSocket Client
│
├── Network/            # C Game Server
│   ├── src/
│   │   ├── handlers/   # Game Logic (room_handler.c, match_handler.c)
│   │   ├── db/         # Database Client (libcurl to Supabase)
│   │   └── main.c      # Server Entry Point
│   ├── include/        # Header Files
│   └── Dockerfile      # C Server Build
│
├── ws-bridge/          # WebSocket <-> TCP Proxy
│   └── src/            # Node.js Bridge Logic
│
├── Database/           # SQL Schema & Migrations
│   ├── init.sql        # Initial Table Setup
│   └── migrations/     # Updates (RPC functions)
│
└── Docs/               # Design Documentation
```

## 🎮 Game Modes

### 1. Scoring Mode (4-6 Players)
*   **Goal**: Accumulate the highest score after 3 rounds.
*   **Mechanic**: Classic point-based competition.

### 2. Elimination Mode (4 Players Fixed)
*   **Goal**: Be the last survivor.
*   **Mechanic**: The player with the lowest score is eliminated after each round.

## 🛠 Technology Details

*   **Communication**: Proprietary Binary Protocol (Header + Payload) over TCP.
*   **Performance**: C Server handles game state with high efficiency.
*   **Persistence**: Game results are stored in PostgreSQL for history and leaderboards.

## 📋 Development

### Network (C Server)
```bash
cd Network
mkdir build && cd build
cmake ..
make
./bin/socket_server
```

### Frontend (React)
```bash
cd Frontend
npm install
npm start
```

## 📜 License
This project is for educational purposes - Network Programming Project.
