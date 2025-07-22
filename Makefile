# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Isrc
SRC_DIR = src
BUILD_DIR = build

SRCS = \
    $(SRC_DIR)/main.c \
    $(SRC_DIR)/lexer/lexer.c \
    $(SRC_DIR)/parser/parser.c \
    $(SRC_DIR)/ast/ast.c \
    $(SRC_DIR)/types/object.c \
    $(SRC_DIR)/types/value.c \
    $(SRC_DIR)/types/variable.c \
    $(SRC_DIR)/interpreter/interpreter.c

# Replace src/xyz.c → build/xyz.o
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

OUT = $(BUILD_DIR)/able_exe

# Default target
all: $(OUT)

# Link
$(OUT): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

run:
	@FILE=$(file); \
	if [ -z "$$FILE" ]; then \
		echo "Usage: make run file=path/to/file.abl"; \
		exit 1; \
	fi; \
	$(OUT) $$FILE