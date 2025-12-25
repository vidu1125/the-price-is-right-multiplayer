# Network/Makefile
CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = -lsqlite3

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Source files
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/server.c \
       $(SRC_DIR)/client_manager.c \
       $(SRC_DIR)/protocol/protocol.c \
       $(SRC_DIR)/handlers/handler_registry.c \
       $(SRC_DIR)/handlers/history/view_history_handler.c

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Target executable
TARGET = $(BIN_DIR)/game_server

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/handlers
	@mkdir -p $(BUILD_DIR)/handlers/history

# Link
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET)"

# Compile .c to .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "🧹 Clean complete"

# Run
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: clean all

# Release build
release: CFLAGS += -O2 -DNDEBUG
release: clean all

# Show compilation commands
verbose: CFLAGS += -v
verbose: all

.PHONY: all clean run debug release verbose directories