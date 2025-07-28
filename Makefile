CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Isrc -Ivendor -D_GNU_SOURCE
LDFLAGS = -lm
SRC_DIR = src
BUILD_DIR = build

SRCS = \
    $(SRC_DIR)/main.c \
    $(SRC_DIR)/lexer/lexer.c \
    $(SRC_DIR)/parser/parser.c \
    $(SRC_DIR)/ast/ast.c \
    $(SRC_DIR)/types/object.c \
    $(SRC_DIR)/types/type.c \
    $(SRC_DIR)/types/type_registry.c \
    $(SRC_DIR)/types/value.c \
    $(SRC_DIR)/types/instance.c \
    $(SRC_DIR)/types/list.c \
    $(SRC_DIR)/types/function.c \
    $(SRC_DIR)/types/env.c \
    $(SRC_DIR)/interpreter/interpreter.c \
    $(SRC_DIR)/interpreter/stack.c \
    $(SRC_DIR)/interpreter/resolve.c \
    $(SRC_DIR)/interpreter/attr.c \
    $(SRC_DIR)/utils/utils.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
OUT = $(BUILD_DIR)/able_exe

all: $(OUT)

$(OUT): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run:
	@FILE=$(file); \
	if [ -z "$$FILE" ]; then \
		echo "Usage: make run file=path/to/file.abl"; \
		exit 1; \
	fi; \
	$(OUT) $$FILE
