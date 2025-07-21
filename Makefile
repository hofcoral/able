CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -std=c99 -D_GNU_SOURCE
SRC_DIR=src
OBJ_DIR=build
BIN_DIR=bin

SRCS=$(shell find $(SRC_DIR) -name '*.c')
OBJS=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

$(BIN_DIR)/able: $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
