#ifndef _MX_SEMA_H
#define _MX_SEMA_H

#include "ast.h"
#include "mem.h"
#include "mxir.h"

Mxir *analyze(Arena *permanent_arena, Ast *ast);

typedef struct {
    const char *name;
    const char *ast_node_type;
} MXEnvBinding;

typedef int32_t MXEnvBindingRef;

typedef int32_t MXEnvRef;

typedef struct {
    MXEnvRef parent_ref;
    HashMap *members;
} MXEnv;

typedef ArrayList(MXEnv) MXEnvList;

typedef struct {
    Arena *permanent_arena;
    Arena arena;
    Ast *ast;
    Mxir *ir;
    MXEnvList *envs;
    MXEnvRef root_env_ref;
    MXEnvRef current_env_ref;
} MXSema;

MXSema *mx_sema_new(Arena *permanent_arena);

#endif // _MX_SEMA_H
