# ============================================================================
# Game Server Makefile - SQLite Version
# ============================================================================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS = -lsqlite3

# Directories
SRC_DIR = src
HANDLER_DIR = $(SRC_DIR)/handlers
OBJ_DIR = obj
BIN_DIR = bin
INC_DIR = include

# Target executable
TARGET = $(BIN_DIR)/game_server

# Source files
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/server.c \
       $(SRC_DIR)/protocol.c \
       $(SRC_DIR)/client_manager.c \
       $(SRC_DIR)/room_manager.c \
       $(SRC_DIR)/database.c \
       $(SRC_DIR)/utils.c \
       $(SRC_DIR)/message_handler.c \
       $(HANDLER_DIR)/auth_handler.c \
       $(HANDLER_DIR)/room_handler.c \
       $(HANDLER_DIR)/game_handler.c \
       $(HANDLER_DIR)/social_handler.c \
       $(HANDLER_DIR)/system_handler.c

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Detect OS and configure paths
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    # macOS - Homebrew paths
    SQLITE3_PREFIX := $(shell brew --prefix sqlite 2>/dev/null)
    ifneq ($(SQLITE3_PREFIX),)
        CFLAGS += -I$(SQLITE3_PREFIX)/include
        LDFLAGS := -L$(SQLITE3_PREFIX)/lib $(LDFLAGS)
    endif
endif

ifeq ($(UNAME_S),Linux)
    # Linux - standard paths usually work
    # Add custom paths if needed
    # CFLAGS += -I/usr/local/include
    # LDFLAGS := -L/usr/local/lib $(LDFLAGS)
endif

# Create directories
$(shell mkdir -p $(OBJ_DIR) $(OBJ_DIR)/handlers $(BIN_DIR))

# ============================================================================
# Build Rules
# ============================================================================

# Default target
all: $(TARGET)
	@echo ""
	@echo "✅ Build successful!"
	@echo "   Run with: make run"
	@echo "   Or:       ./$(TARGET)"

# Link object files to create executable
$(TARGET): $(OBJS)
	@echo ""
	@echo "🔗 Linking $(TARGET)..."
	@$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "📦 Compiling $<..."
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# Utility Targets
# ============================================================================

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "✅ Clean complete"

# Deep clean (including database)
distclean: clean
	@echo "🧹 Removing database..."
	@rm -f game_server.db game_server.db-journal
	@echo "✅ Deep clean complete"

# Run the server
run: $(TARGET)
	@echo ""
	@echo "🚀 Starting server..."
	@echo ""
	@./$(TARGET)

# Rebuild everything
rebuild: clean all

# ============================================================================
# Development Targets
# ============================================================================

# Debug build with extra flags
debug: CFLAGS += -DDEBUG -O0 -ggdb3 -fsanitize=address -fsanitize=undefined
debug: LDFLAGS += -fsanitize=address -fsanitize=undefined
debug: rebuild
	@echo "🐛 Debug build complete"

# Release build with optimization
release: CFLAGS += -O2 -DNDEBUG -flto
release: LDFLAGS += -flto
release: rebuild
	@echo "🚀 Release build complete"

# Verbose build (show full commands)
verbose: MAKEFLAGS += --no-print-directory
verbose:
	@$(MAKE) all V=1

# Check code style (if clang-format installed)
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "🎨 Formatting code..."; \
		find $(SRC_DIR) $(INC_DIR) -name "*.c" -o -name "*.h" | xargs clang-format -i; \
		echo "✅ Format complete"; \
	else \
		echo "⚠️  clang-format not installed"; \
	fi

# Static analysis (if cppcheck installed)
check:
	@if command -v cppcheck >/dev/null 2>&1; then \
		echo "🔍 Running static analysis..."; \
		cppcheck --enable=all --suppress=missingIncludeSystem $(SRC_DIR); \
		echo "✅ Check complete"; \
	else \
		echo "⚠️  cppcheck not installed"; \
	fi

# ============================================================================
# Database Management
# ============================================================================

# View database schema
db-schema:
	@echo "📊 Database schema:"
	@sqlite3 game_server.db ".schema" 2>/dev/null || echo "❌ Database not found. Run server first."

# View users table
db-users:
	@echo "👥 Users:"
	@sqlite3 game_server.db "SELECT * FROM users;" 2>/dev/null || echo "❌ No users found"

# View match history
db-history:
	@echo "📜 Match history:"
	@sqlite3 game_server.db "SELECT * FROM match_history;" 2>/dev/null || echo "❌ No matches found"

# Interactive database shell
db-shell:
	@sqlite3 game_server.db

# Backup database
db-backup:
	@echo "💾 Backing up database..."
	@cp game_server.db game_server.db.backup.$(shell date +%Y%m%d_%H%M%S)
	@echo "✅ Backup complete"

# Reset database (WARNING: deletes all data)
db-reset:
	@echo "⚠️  This will delete all data!"
	@read -p "Are you sure? [y/N] " -n 1 -r; \
	echo; \
	if [[ $$REPLY =~ ^[Yy]$$ ]]; then \
		rm -f game_server.db game_server.db-journal; \
		echo "✅ Database reset"; \
	else \
		echo "❌ Cancelled"; \
	fi

# ============================================================================
# Testing & Monitoring
# ============================================================================

# Run with valgrind (memory leak detection)
valgrind: debug
	@if command -v valgrind >/dev/null 2>&1; then \
		echo "🔍 Running with valgrind..."; \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET); \
	else \
		echo "⚠️  valgrind not installed"; \
	fi

# Check if server is running
status:
	@echo "📊 Server status:"
	@if lsof -i :5500 >/dev/null 2>&1; then \
		echo "✅ Server is running"; \
		lsof -i :5500; \
	else \
		echo "❌ Server is not running"; \
	fi

# Kill running server
kill:
	@echo "🔪 Killing server..."
	@pkill -f game_server || echo "No server process found"
	@echo "✅ Done"

# Watch for file changes and rebuild (requires fswatch)
watch:
	@if command -v fswatch >/dev/null 2>&1; then \
		echo "👀 Watching for changes..."; \
		fswatch -o $(SRC_DIR) $(INC_DIR) | xargs -n1 -I{} make rebuild; \
	else \
		echo "⚠️  fswatch not installed"; \
		echo "Install: brew install fswatch"; \
	fi

# ============================================================================
# Installation & Dependencies
# ============================================================================

# Install dependencies (macOS)
deps-macos:
	@echo "📦 Installing dependencies for macOS..."
	@brew install sqlite

# Install dependencies (Ubuntu/Debian)
deps-linux:
	@echo "📦 Installing dependencies for Linux..."
	@sudo apt-get update
	@sudo apt-get install -y sqlite3 libsqlite3-dev gcc make

# Check dependencies
check-deps:
	@echo "🔍 Checking dependencies..."
	@command -v $(CC) >/dev/null 2>&1 && echo "✅ gcc found" || echo "❌ gcc not found"
	@command -v sqlite3 >/dev/null 2>&1 && echo "✅ sqlite3 found" || echo "❌ sqlite3 not found"
	@pkg-config --exists sqlite3 2>/dev/null && echo "✅ libsqlite3-dev found" || echo "❌ libsqlite3-dev not found"

# ============================================================================
# Information & Help
# ============================================================================

# Show project info
info:
	@echo "╔══════════════════════════════════════════════════════╗"
	@echo "║          Game Server - Project Information          ║"
	@echo "╠══════════════════════════════════════════════════════╣"
	@echo "║ Database:     SQLite3                                ║"
	@echo "║ Compiler:     $(CC)                                    ║"
	@echo "║ Port:         5500                                   ║"
	@echo "║ Max Clients:  64                                     ║"
	@echo "║ Max Rooms:    16                                     ║"
	@echo "╠══════════════════════════════════════════════════════╣"
	@echo "║ Source files: $(words $(SRCS))                       ║"
	@echo "║ Object files: $(words $(OBJS))                       ║"
	@echo "╚══════════════════════════════════════════════════════╝"

# Help target
help:
	@echo ""
	@echo "╔══════════════════════════════════════════════════════╗"
	@echo "║          Game Server Makefile - Help                ║"
	@echo "╚══════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Build Targets:"
	@echo "  make              - Build the server (default)"
	@echo "  make all          - Same as 'make'"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean    - Remove build artifacts + database"
	@echo "  make rebuild      - Clean and build"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make release      - Build optimized release version"
	@echo ""
	@echo "Run Targets:"
	@echo "  make run          - Build and run the server"
	@echo "  make status       - Check if server is running"
	@echo "  make kill         - Kill running server"
	@echo ""
	@echo "Database Targets:"
	@echo "  make db-schema    - Show database schema"
	@echo "  make db-users     - List all users"
	@echo "  make db-history   - Show match history"
	@echo "  make db-shell     - Open SQLite shell"
	@echo "  make db-backup    - Backup database"
	@echo "  make db-reset     - Reset database (deletes all data)"
	@echo ""
	@echo "Development Targets:"
	@echo "  make format       - Format code with clang-format"
	@echo "  make check        - Run static analysis"
	@echo "  make valgrind     - Run with valgrind"
	@echo "  make watch        - Auto-rebuild on file changes"
	@echo ""
	@echo "Utility Targets:"
	@echo "  make check-deps   - Check if dependencies installed"
	@echo "  make deps-macos   - Install dependencies (macOS)"
	@echo "  make deps-linux   - Install dependencies (Linux)"
	@echo "  make info         - Show project information"
	@echo "  make help         - Show this help message"
	@echo ""

# Verbose mode support
ifdef V
    QUIET =
else
    QUIET = @
endif

# ============================================================================
# Phony Targets
# ============================================================================

.PHONY: all clean distclean run rebuild debug release verbose format check \
        db-schema db-users db-history db-shell db-backup db-reset \
        valgrind status kill watch \
        deps-macos deps-linux check-deps info help

# ============================================================================
# Default target if make is called without arguments
# ============================================================================
.DEFAULT_GOAL := all