# ==============================
# Compiler & flags
# ==============================
CC := gcc
CFLAGS := -Wall -Wextra -O2 -g
INCLUDES := -Iinclude

# ==============================
# Directories
# ==============================
SRC_DIR := src
BUILD_DIR := build

# ==============================
# Source files
# ==============================
SRCS := \
    $(wildcard $(SRC_DIR)/*.c) \
    $(wildcard $(SRC_DIR)/transport/*.c) \
    $(wildcard $(SRC_DIR)/handlers/*.c)

# message.c có thể bỏ nếu bạn không dùng helper protocol
# Nếu không có file này, xóa dòng trên

OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# ==============================
# Output
# ==============================
TARGET := network_server

# ==============================
# Rules
# ==============================
.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
