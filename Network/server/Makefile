# Network/server/Makefile

CC = gcc
CFLAGS = -Wall -Wextra -I./include -I../protocol
LDFLAGS = -lcjson -lpthread

SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
PROTOCOL_DIR = ../protocol

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

TARGET = $(BUILD_DIR)/server

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	@echo "🧹 Clean complete"

run: $(TARGET)
	./$(TARGET)

.PHONY: test
test:
	@echo "Running tests..."
	# Add test commands here
