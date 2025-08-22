##
# \file Makefile
# \author Isaiah Lateer
# 
# Used to build the project and clean the workspace.
# 

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SRC_FILES := $(shell find $(SRC_DIR) -name '*.c')
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
BIN_FILES := $(BIN_DIR)/opengl_context.exe

all: $(BIN_FILES)

$(BIN_FILES): $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	gcc -o $@ $^ -lX11 -lGL

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	gcc -std=c11 -Wall -Werror -DNDEBUG -Isrc -Iinclude -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
