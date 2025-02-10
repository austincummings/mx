# Compiler and tools
CC := clang
AR := ar

# Include directories
INCLUDES := -I. -Ithird_party/jansson/include

# Compiler and linker flags
CFLAGS := -std=c23 -Wall -Wextra -pedantic -flto -g -static $(INCLUDES)
LDFLAGS := -Ltree-sitter-mx -Lthird_party/tree-sitter -Lthird_party/jansson/build/lib -static -flto
LIBS := -ltree-sitter-mx -ltree-sitter -ljansson

# Directories
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
LIB_DIR := lib

# Source and object files
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
LIB_OBJECTS := $(filter-out $(OBJ_DIR)/main.o, $(OBJECTS))

# Targets
BIN_TARGET := $(BIN_DIR)/mxc
LIB_TARGET := $(LIB_DIR)/libmx.a

# Default target
.PHONY: all clean grammar
all: grammar $(BIN_TARGET) $(LIB_TARGET)

grammar:
	@cd tree-sitter-mx && tree-sitter generate && $(MAKE)

# Rule to link the executable
$(BIN_TARGET): $(OBJECTS) $(LIB_TARGET) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIB_TARGET) $(LIBS)

# Rule to build the static library
$(LIB_TARGET): $(LIB_OBJECTS) | $(LIB_DIR)
	@echo "Building static library $@"
	$(AR) rcs $@ $(LIB_OBJECTS)

# Rule to compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure necessary directories exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

# Clean up build files
clean:
	rm -f $(BIN_TARGET) $(LIB_TARGET) $(OBJECTS)
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(LIB_DIR)
	rm -rf out
