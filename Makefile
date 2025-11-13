CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -D_POSIX_C_SOURCE=200809L -Iinclude -Isrc
LDFLAGS = -lm -lpthread -lffi -ldl
SRC_DIR = src
BUILD_DIR = build

# Source files from src/ and src/interpreter/ and src/interpreter/builtins/ and src/interpreter/io/
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/interpreter/*.c) $(wildcard $(SRC_DIR)/interpreter/builtins/*.c) $(wildcard $(SRC_DIR)/interpreter/io/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
TARGET = hemlock

all: $(BUILD_DIR) $(BUILD_DIR)/interpreter $(BUILD_DIR)/interpreter/builtins $(BUILD_DIR)/interpreter/io $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/interpreter:
	mkdir -p $(BUILD_DIR)/interpreter

$(BUILD_DIR)/interpreter/builtins:
	mkdir -p $(BUILD_DIR)/interpreter/builtins

$(BUILD_DIR)/interpreter/io:
	mkdir -p $(BUILD_DIR)/interpreter/io

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@bash tests/run_tests.sh

.PHONY: all clean run test