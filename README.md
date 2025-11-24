# The Price Is Right - Multiplayer

> Dự án môn Lập trình mạng  
> Network: C/C++ | Backend: Python | Frontend: React

##  Quick Start với Docker

### Prerequisites
- Docker Desktop

### Setup & Run
```bash
# 1. Clone repository
git clone <repo-url>
cd the-price-is-right-multiplayer

# 2. Chạy tất cả services
docker-compose up --build

# 3. Truy cập
# Frontend: http://localhost:3000
# Backend:  http://localhost:5000/health
# Network:  localhost:8888
```

### Stop Services
```bash
docker-compose down
```

##  Architecture

- **Network Layer** (C): Port 8888 - Custom socket protocol
- **Backend API** (Python): Port 5000 - Game logic
- **Frontend** (React): Port 3000 - UI
- **Database** (PostgreSQL): Port 5432

##  Game Rules

5 Rounds:
1. Quiz Round
2. Bid Round  
3. Up or Down
4. Spin Bonus
5. Solo Battle

## ⚖️ Compliance

 Custom C/C++ Socket (no libraries)  
 Manual protocol implementation  
 No socket.io/WebSocket
