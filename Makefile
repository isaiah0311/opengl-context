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
OBJ_FILES := $(patsubst %.c,$(OBJ_DIR)/%.obj,$(notdir $(SRC_FILES)))
BIN_FILES := $(BIN_DIR)/opengl_context.exe

all: $(BIN_FILES)

$(BIN_FILES): $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	gcc -o $@ $^ -lX11 -lGL

define getsrc
$(firstword $(filter %/$(1:.obj=.c),$(SRC_FILES)))
endef

$(OBJ_FILES): $(OBJ_DIR)/%.obj:
	@mkdir -p $(OBJ_DIR)
	$(eval SRC_FILE := $(call getsrc,$(@F)))
	@if [ -z "$(SRC_FILE)" ]; then \
		echo "[ERROR] Could not find source file for $@."; \
		exit 1; \
	fi
	gcc -std=c11 -Wall -Werror -DNDEBUG -Isrc -Iinclude -c $(SRC_FILE) -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
