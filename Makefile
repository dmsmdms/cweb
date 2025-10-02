# Compiler
CC := gcc
CFLAGS := -Wall -Wextra -O2 -I/usr/include/json-c -g -Og
CFLAGS := $(CFLAGS) -Iinclude

# Directories
SRC_DIR := src
OBJ_DIR := build
BIN := build/app

# Find all source files
SRCS := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/core/*.c) $(wildcard $(SRC_DIR)/db/*.c) $(wildcard $(SRC_DIR)/parser/*.c)
# Create corresponding object files in OBJ_DIR
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Default target
all: $(BIN)

# Link objects into executable
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -ljson-c -lcurl -lgumbo

# Compile sources into objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure object directory exists
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR) $(OBJ_DIR)/core $(OBJ_DIR)/db $(OBJ_DIR)/parser

# Clean build files
clean:
	rm -rf $(OBJ_DIR) $(BIN)

.PHONY: all clean
