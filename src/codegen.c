#include "codegen.h"
#include "mem.h"

MXCodeGen *mx_codegen_new(Arena *arena) {
    MXCodeGen *codegen = arena_alloc_struct(arena, MXCodeGen);
    codegen->src = NULL;
    return codegen;
}
