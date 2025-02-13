#ifndef _MX_SEMA_H
#define _MX_SEMA_H

#include "ast.h"
#include "mem.h"
#include "mxir.h"

typedef struct {
    char *src;
} MXSema;

MXSema *mx_sema_new(Arena *arena);

Mxir *analyze(Arena *permanent_arena, Ast *ast);

#endif // _MX_SEMA_H
