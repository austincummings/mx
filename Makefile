# Compiler to use
CC := clang

# Include directories
INCLUDES := -I.

# Compiler flags
CFLAGS := -std=c23 -Wall -Wextra -Werror -pedantic -Wstrict-prototypes -g $(INCLUDES)

# Linker flags
TREE_SITTER_MX_DIR := tree-sitter-mx
TREE_SITTER_DIR := /home/austin/src/tree-sitter
LDFLAGS := -L. -L$(TREE_SITTER_MX_DIR) -L$(TREE_SITTER_DIR) -static

# Libraries to link
LIBS := -ltree-sitter-mx -ltree-sitter

# Source files
SOURCES := $(wildcard src/*.c)

# Object files to build
OBJECTS := $(SOURCES:.c=.o)
LIB_OBJECTS := $(filter-out main.o, $(SOURCES:.c=.o))

# Executable to build
BIN_TARGET := mx
LIB_TARGET := libmx.a

# Default target
all: grammar $(BIN_TARGET) $(LIB_TARGET)

.PHONY: all clean grammar

grammar:
	@cd $(TREE_SITTER_MX_DIR) && tree-sitter generate && make

# Rule to link the program
$(BIN_TARGET): $(OBJECTS) $(LIB_TARGET)
	$(CC) $(LDFLAGS) -o $@ $(LIB_TARGET)

$(LIB_TARGET): $(LIB_OBJECTS)
	@echo "Building static library $(LIB_TARGET)"
	cd $(TREE_SITTER_MX_DIR) && ar x libtree-sitter-mx.a
	cd $(TREE_SITTER_DIR) && ar x libtree-sitter.a
	ar rcs $@ $(LIB_OBJECTS) $(TREE_SITTER_MX_DIR)/*.o $(TREE_SITTER_DIR)/*.o
	rm -f $(TREE_SITTER_MX_DIR)/*.o $(TREE_SITTER_DIR)/*.o

# Rule to compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(BIN_TARGET) $(LIB_TARGET) $(OBJECTS)
	rm -rf out
