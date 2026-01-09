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
    $(wildcard $(SRC_DIR)/handlers/*.c) \
	$(wildcard $(SRC_DIR)/utils/*.c) \
	$(wildcard $(SRC_DIR)/db/repo/*.c) \
	$(wildcard $(SRC_DIR)/db/core/*.c)


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
LIBS := -lcurl -lcjson -lcrypt -luuid

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
