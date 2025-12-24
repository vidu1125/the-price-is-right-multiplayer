# The Price Is Right - Multiplayer Game

A multiplayer price guessing game with a custom network protocol implementation using C/C++ sockets, Python backend, and React frontend.

## Table of Contents

- [Overview](#overview)
- [Technology Stack](#technology-stack)
- [System Architecture](#system-architecture)
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Development Setup](#development-setup)
- [Docker Management](#docker-management)
- [Project Structure](#project-structure)
- [Game Rules](#game-rules)
- [Testing](#testing)
- [Git Workflow](#git-workflow)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

This project implements a multiplayer game server using a custom network protocol built from scratch in C/C++. The game combines real-time socket communication with modern web technologies to deliver an interactive multiplayer experience.

**Key Features:**
- Custom TCP/UDP network protocol in C/C++
- Real-time multiplayer gameplay
- WebSocket communication for instant updates
- Dockerized microservices architecture
- PostgreSQL data persistence

---

## Technology Stack

| Layer | Technology | Purpose |
|-------|-----------|---------|
| **Network** | C/C++ Sockets | Custom TCP/UDP server implementation |
| **Backend** | Python 3.11+ | Game engine and business logic |
| **Frontend** | React + Node.js | Interactive web UI |
| **Database** | PostgreSQL | Game state and user data storage |
| **IPC** | Unix Sockets/Named Pipes | Inter-process communication |
| **Container** | Docker + Docker Compose | Service orchestration |

---

## System Architecture

```
┌──────────────────┐
│   Web Browser    │  WebSocket Connection
└────────┬─────────┘
         │
┌────────▼─────────┐
│  React Frontend  │  Port 3000
└────────┬─────────┘
         │ HTTP/REST
┌────────▼─────────┐
│  Python Backend  │  Port 5000
│  (Game Logic)    │
└────────┬─────────┘
         │ IPC Bridge
┌────────▼─────────┐
│  C Socket Server │  Port 8888
│ (Custom Protocol)│
└────────┬─────────┘
         │ TCP/UDP
┌────────▼─────────┐
│  Game Clients    │
└──────────────────┘
         │
┌────────▼─────────┐
│   PostgreSQL DB  │  Port 5432
└──────────────────┘
```

---

## Prerequisites

Ensure the following tools are installed on your system:

**Required:**
- **Git** >= 2.0 - [Download](https://git-scm.com/downloads)
- **Docker** >= 20.10 - [Install Guide](https://docs.docker.com/get-docker/)
- **Docker Compose** >= 2.0 - [Install Guide](https://docs.docker.com/compose/install/)

**Optional (for local development):**
- **GCC/G++** >= 9.0 (for C/C++ development)
- **Python** >= 3.11 (for backend development)
- **Node.js** >= 18.0 (for frontend development)

**Verify Installation:**
```bash
git --version
docker --version
docker-compose --version
```

---

## Quick Start

Get the project running in under 3 minutes:

### 1. Clone Repository

```bash
git clone https://github.com/vidu1125/the-price-is-right-multiplayer.git
cd the-price-is-right-multiplayer
```

### 2. Start All Services

```bash
docker-compose up --build
```

Wait for all services to initialize (approximately 2-3 minutes on first run).

### 3. Access Application

- **Frontend:** http://localhost:3000
- **Backend API:** http://localhost:5000/health
- **Socket Server:** localhost:8888



## Multiplayer enabling
ngrok config add-authtoken [Your_token_here]
ngrok http 3000
---

## Development Setup

### Local Development Without Docker

#### Backend Setup (Python)

```bash
# Navigate to backend directory
cd Backend

# Create virtual environment
python3 -m venv venv

# Activate virtual environment
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Run backend server
python app/main.py
```

#### Frontend Setup (React)

```bash
# Navigate to frontend directory
cd Frontend

# Install dependencies
npm install

# Start development server
npm start
```

#### Network Layer Setup (C/C++)

```bash
# Navigate to network directory
cd Network

# Create build directory
mkdir build && cd build

# Build with CMake
cmake ..
make

# Run socket server
./bin/socket_server
```

### Environment Variables

Create a `.env` file in the root directory:

```env
# Database Configuration
POSTGRES_USER=gameuser
POSTGRES_PASSWORD=gamepass123
POSTGRES_DB=price_game_db
DATABASE_URL=postgresql://gameuser:gamepass123@db:5432/price_game_db

# Backend Configuration
BACKEND_PORT=5000
FLASK_ENV=development

# Network Configuration
SOCKET_SERVER_PORT=8888
SOCKET_SERVER_HOST=0.0.0.0

# Frontend Configuration
REACT_APP_API_URL=http://localhost:5000
REACT_APP_WS_URL=ws://localhost:8888
```

---

## Docker Management

### Basic Commands

#### Start Services

```bash
# Start all services (foreground)
docker-compose up

# Start all services (background/detached)
docker-compose up -d

# Start with rebuild
docker-compose up --build

# Start specific service
docker-compose up backend
```

#### Stop Services

```bash
# Stop all services
docker-compose down

# Stop and remove volumes
docker-compose down -v

# Stop and remove everything (including images)
docker-compose down --rmi all -v
```

#### View Container Status

```bash
# List running containers
docker-compose ps

# View all containers (including stopped)
docker ps -a

# View resource usage
docker stats
```

#### View Logs

```bash
# View logs from all services
docker-compose logs

# Follow logs in real-time
docker-compose logs -f

# View logs from specific service
docker-compose logs -f backend

# View last 100 lines
docker-compose logs --tail=100
```

#### Execute Commands in Containers

```bash
# Open bash shell in backend container
docker-compose exec backend bash

# Run Python command in backend
docker-compose exec backend python -c "print('Hello')"

# Access PostgreSQL
docker-compose exec db psql -U gameuser -d price_game_db
```

#### Restart Services

```bash
# Restart all services
docker-compose restart

# Restart specific service
docker-compose restart backend
```

#### Clean Up

```bash
# Remove unused containers
docker container prune

# Remove unused images
docker image prune

# Remove unused volumes
docker volume prune

# Remove everything unused
docker system prune -a
```

### Advanced Docker Operations

#### Rebuild Single Service

```bash
# Rebuild and restart backend only
docker-compose up -d --no-deps --build backend
```

#### Scale Services

```bash
# Run 3 instances of backend
docker-compose up -d --scale backend=3
```

#### View Service Configuration

```bash
# Show resolved configuration
docker-compose config
```

#### Export/Import Container Data

```bash
# Export container
docker export backend_container > backend_backup.tar

# Import container
docker import backend_backup.tar
```

---

## Project Structure

```
the-price-is-right-multiplayer/
├── Backend/                    # Python backend service
│   ├── app/
│   │   ├── __init__.py
│   │   ├── main.py            # Flask application entry point
│   │   ├── game_engine/       # Game logic implementation
│   │   ├── models/            # Database models
│   │   └── services/          # Business logic services
│   ├── requirements.txt       # Python dependencies
│   └── Dockerfile
├── Frontend/                   # React frontend application
│   ├── public/
│   │   └── index.html
│   ├── src/
│   │   ├── App.js             # Main React component
│   │   ├── index.js           # Application entry point
│   │   └── components/        # React components
│   ├── package.json           # Node.js dependencies
│   └── Dockerfile
├── Network/                    # C/C++ socket implementation
│   ├── server/
│   │   ├── src/
│   │   │   └── socket_server.c
│   │   └── Dockerfile
│   ├── client/
│   │   └── src/
│   ├── protocol/
│   │   └── packet_format.h    # Custom protocol definition
│   ├── tests/                 # Network tests
│   └── CMakeLists.txt
├── Database/
│   └── init.sql               # Database initialization scripts
├── Scripts/                    # Utility scripts
├── Docs/                       # Project documentation
├── API/                        # API documentation
├── docker-compose.yml          # Service orchestration
├── .dockerignore
├── .gitignore
└── README.md
```

---

## Game Rules

The game consists of five rounds with different mechanics:

### Round 1: Quiz Round
- Players answer trivia questions
- Correct answers earn points
- Time limit: 30 seconds per question

### Round 2: Bid Round
- Guess the exact price of an item
- Closest guess without going over wins
- Overbids are eliminated

### Round 3: Up or Down
- Shown a product and price
- Decide if actual price is higher or lower
- Consecutive correct answers increase multiplier

### Round 4: Spin Bonus
- Virtual wheel spin for bonus points
- Landing zones: 5, 10, 15, 20, 25, 50, 100 points
- One spin per player

### Round 5: Solo Battle
- Head-to-head price guessing
- Best of 3 rounds
- Winner takes all remaining points

---

## Testing

### Run All Tests

```bash
# Using Docker
docker-compose run --rm backend pytest
docker-compose run --rm frontend npm test
```

### Backend Tests (Python)

```bash
cd Backend
source venv/bin/activate

# Run all tests
pytest

# Run with coverage
pytest --cov=app --cov-report=html

# Run specific test file
pytest tests/test_game_engine.py

# Run with verbose output
pytest -v
```

### Frontend Tests (React)

```bash
cd Frontend

# Run tests
npm test

# Run with coverage
npm test -- --coverage

# Run in watch mode
npm test -- --watch
```

### Network Layer Tests (C/C++)

```bash
cd Network/tests

# Compile test
gcc test_socket.c -o test_socket

# Run test
./test_socket

# Run with valgrind (memory check)
valgrind --leak-check=full ./test_socket
```

### Integration Tests

```bash
# Ensure all services are running
docker-compose up -d

# Run integration tests
docker-compose exec backend pytest tests/integration/
```

---

## Git Workflow

### Initial Setup

```bash
# Clone repository
git clone https://github.com/vidu1125/the-price-is-right-multiplayer.git
cd the-price-is-right-multiplayer

# Configure user
git config user.name "Your Name"
git config user.email "your.email@example.com"
```

### Feature Development

```bash
# Create and switch to new feature branch
git checkout -b feature/your-feature-name

# Make changes to code
# ...

# Check status
git status

# Stage changes
git add .

# Commit with descriptive message
git commit -m "Add: implement user authentication"

# Push to remote
git push origin feature/your-feature-name
```

### Commit Message Convention

Use clear, descriptive commit messages:

```
Add: new feature
Fix: bug fix
Update: modify existing feature
Remove: delete code/files
Refactor: code restructuring
Docs: documentation changes
Test: add or modify tests
Style: formatting, missing semicolons, etc.
```

Examples:
```bash
git commit -m "Add: socket connection pooling"
git commit -m "Fix: memory leak in packet parser"
git commit -m "Update: improve game round timer accuracy"
```

### Syncing with Main Branch

```bash
# Switch to main branch
git checkout main

# Pull latest changes
git pull origin main

# Switch back to feature branch
git checkout feature/your-feature-name

# Merge main into feature branch
git merge main

# Resolve conflicts if any
# Push updated branch
git push origin feature/your-feature-name
```

### Pull Request Process

1. Push your feature branch to GitHub
2. Go to GitHub repository
3. Click "New Pull Request"
4. Select your feature branch
5. Add description of changes
6. Request review from team members
7. Address review comments
8. Merge after approval

---

## Troubleshooting

### Docker Issues

**Problem: Docker daemon not running**
```bash
# Check Docker status
sudo systemctl status docker

# Start Docker
sudo systemctl start docker

# Enable Docker on boot
sudo systemctl enable docker
```

**Problem: Permission denied**
```bash
# Add user to docker group
sudo usermod -aG docker $USER

# Apply group changes (logout and login)
newgrp docker
```

**Problem: Port already in use**
```bash
# Find process using port 8888
sudo lsof -i :8888

# Kill the process
sudo kill -9 <PID>

# Or change port in docker-compose.yml
```

**Problem: Out of disk space**
```bash
# Clean up Docker system
docker system prune -a -f

# Remove unused volumes
docker volume prune -f

# Check disk usage
docker system df
```

### Build Failures

**Problem: Backend build fails**
```bash
# Clear Python cache
find . -type d -name __pycache__ -exec rm -rf {} +
find . -type f -name "*.pyc" -delete

# Rebuild without cache
docker-compose build --no-cache backend
```

**Problem: Frontend build fails**
```bash
# Clear node_modules and package-lock
cd Frontend
rm -rf node_modules package-lock.json

# Reinstall dependencies
npm install

# Rebuild container
docker-compose build --no-cache frontend
```

**Problem: Network layer compilation errors**
```bash
# Check GCC version
gcc --version

# Clean build directory
cd Network
rm -rf build
mkdir build && cd build

# Reconfigure and build
cmake ..
make clean
make
```

### Runtime Errors

**Problem: Cannot connect to database**
```bash
# Check database container status
docker-compose ps db

# Check database logs
docker-compose logs db

# Restart database
docker-compose restart db

# Verify connection
docker-compose exec db psql -U gameuser -d price_game_db
```

**Problem: Backend API not responding**
```bash
# Check backend logs
docker-compose logs -f backend

# Restart backend
docker-compose restart backend

# Check backend health
curl http://localhost:5000/health
```

**Problem: Socket server not accepting connections**
```bash
# Check if port is listening
netstat -tuln | grep 8888

# Check firewall rules
sudo ufw status

# Test socket connection
telnet localhost 8888
```

### Performance Issues

**Problem: Slow container startup**
```bash
# Check system resources
docker stats

# Increase Docker memory limit (Docker Desktop)
# Settings > Resources > Memory > 4GB+

# Remove unused images
docker image prune -a
```

**Problem: High CPU usage**
```bash
# Identify resource-heavy containers
docker stats --no-stream

# Limit container resources in docker-compose.yml
services:
  backend:
    deploy:
      resources:
        limits:
          cpus: '0.5'
          memory: 512M
```

### Common Error Messages

**"Error response from daemon: Conflict"**
```bash
# Container name already exists
docker rm -f <container_name>
docker-compose up
```

**"no space left on device"**
```bash
# Clean up Docker
docker system prune -a --volumes -f

# Check disk space
df -h
```

**"network not found"**
```bash
# Recreate Docker networks
docker-compose down
docker network prune
docker-compose up
```

---

## Contributing

We welcome contributions! Please follow these guidelines:

### Code Style

- **Python**: Follow PEP 8 style guide
- **JavaScript/React**: Use ESLint configuration
- **C/C++**: Follow Google C++ Style Guide

### Before Submitting

1. Run all tests and ensure they pass
2. Update documentation if needed
3. Add tests for new features
4. Follow commit message conventions
5. Keep pull requests focused and small

### Review Process

1. Submit pull request
2. Automated tests must pass
3. Code review by 2 team members
4. Address review comments
5. Merge after approval

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## Support

- **Issues**: [GitHub Issues](https://github.com/vidu1125/the-price-is-right-multiplayer/issues)
- **Documentation**: Check the `/Docs` directory
- **Contact**: Open an issue for questions

---

## Acknowledgments

Built with:
- Flask and FastAPI for Python backend
- React and Material-UI for frontend
- PostgreSQL for data persistence
- Docker for containerization

---

**Last Updated**: November 2025 
**Version**: 1.0.0
