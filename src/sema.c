#include "sema.h"
#include "mem.h"

static void translate(MXSema *sema) {}

MXSema *mx_sema_new(Arena *permanent_arena, Arena *arena) {
    MXSema *sema = arena_alloc_struct(arena, MXSema);
    sema->permanent_arena = permanent_arena;
    sema->arena = (Arena){0};
    arena_init(&sema->arena, ARENA_DEFAULT_RESERVE_SIZE);
    return sema;
}

Mxir *analyze(Arena *permanent_arena, Ast *ast) {
    Arena arena = {0};
    arena_init(&arena, ARENA_DEFAULT_RESERVE_SIZE);

    MXSema *sema = mx_sema_new(permanent_arena, &arena);
    sema->ast = ast;
    sema->ir = arena_alloc_struct(permanent_arena, Mxir);

    arena_release(&arena);

    return sema->ir;
}
