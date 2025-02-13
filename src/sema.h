#ifndef _MX_SEMA_H
#define _MX_SEMA_H

#include "ast.h"
#include "mem.h"
#include "mxir.h"

typedef struct {
    Arena *permanent_arena;
    Arena arena;
    Ast *ast;
    Mxir *ir;
} MXSema;

MXSema *mx_sema_new(Arena *permanent_arena, Arena *arena);

Mxir *analyze(Arena *permanent_arena, Ast *ast);

#endif // _MX_SEMA_H
