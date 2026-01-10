CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Isrc -Ivendor -Ivendor/mpdecimal -D_GNU_SOURCE -DCONFIG_64 -DANSI -DHAVE_UINT128_T
LDFLAGS = -lm
SRC_DIR = src
BUILD_DIR = build

SRC_SRCS = \
    $(SRC_DIR)/main.c \
    $(SRC_DIR)/lexer/lexer.c \
    $(SRC_DIR)/parser/parser.c \
    $(SRC_DIR)/ast/ast.c \
    $(SRC_DIR)/types/object.c \
    $(SRC_DIR)/types/type.c \
    $(SRC_DIR)/types/value.c \
    $(SRC_DIR)/types/number.c \
    $(SRC_DIR)/types/builtin_types.c \
    $(SRC_DIR)/types/promise.c \
    $(SRC_DIR)/types/instance.c \
    $(SRC_DIR)/types/list.c \
    $(SRC_DIR)/types/env.c \
    $(SRC_DIR)/interpreter/call.c \
    $(SRC_DIR)/interpreter/interpreter.c \
    $(SRC_DIR)/interpreter/annotations.c \
    $(SRC_DIR)/interpreter/module.c \
    $(SRC_DIR)/interpreter/builtins.c \
    $(SRC_DIR)/interpreter/server.c \
    $(SRC_DIR)/interpreter/network.c \
    $(SRC_DIR)/interpreter/stack.c \
    $(SRC_DIR)/interpreter/resolve.c \
    $(SRC_DIR)/interpreter/attr.c \
    $(SRC_DIR)/utils/http_fixtures.c \
    $(SRC_DIR)/utils/http_client.c \
    $(SRC_DIR)/utils/http_server.c \
    $(SRC_DIR)/utils/json.c \
    $(SRC_DIR)/utils/utils.c

VENDOR_SRCS = \
    vendor/mpdecimal/basearith.c \
    vendor/mpdecimal/constants.c \
    vendor/mpdecimal/context.c \
    vendor/mpdecimal/convolute.c \
    vendor/mpdecimal/crt.c \
    vendor/mpdecimal/difradix2.c \
    vendor/mpdecimal/fnt.c \
    vendor/mpdecimal/fourstep.c \
    vendor/mpdecimal/io.c \
    vendor/mpdecimal/mpalloc.c \
    vendor/mpdecimal/mpdecimal.c \
    vendor/mpdecimal/mpsignal.c \
    vendor/mpdecimal/numbertheory.c \
    vendor/mpdecimal/sixstep.c \
    vendor/mpdecimal/transpose.c

SRC_OBJS = $(SRC_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
VENDOR_OBJS = $(VENDOR_SRCS:vendor/%.c=$(BUILD_DIR)/vendor/%.o)
OBJS = $(SRC_OBJS) $(VENDOR_OBJS)
OUT = $(BUILD_DIR)/able_exe

all: $(OUT)

$(OUT): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vendor/%.o: vendor/%.c
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
