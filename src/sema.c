#include "sema.h"

MXSema *mx_sema_new(Arena *arena) {
    MXSema *sema = arena_alloc_struct(arena, MXSema);
    sema->src = NULL;
    return sema;
}

Mxir *analyze(Arena *permanent_arena, Ast *ast) {
    Mxir *ir = arena_alloc_struct(permanent_arena, Mxir);
    return ir;
}
