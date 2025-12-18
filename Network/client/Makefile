# Makefile for C Terminal Client

CC = gcc
CFLAGS = -Wall -Wextra -I../../protocol -std=c11
TARGET = client
SRC_DIR = src
BUILD_DIR = build

SOURCES = $(SRC_DIR)/client_main.c
OBJECTS = $(BUILD_DIR)/client_main.o

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES)
	@echo "✅ Client built successfully: $(BUILD_DIR)/$(TARGET)"

clean:
	rm -rf $(BUILD_DIR)
	@echo "🧹 Cleaned build directory"

run: all
	@echo "\n🚀 Running C Client..."
	@./$(BUILD_DIR)/$(TARGET)

.PHONY: all clean run
