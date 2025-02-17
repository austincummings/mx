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

typedef uint32_t MXEnvBindingRef;

typedef uint32_t MXEnvRef;

typedef struct {
    MXEnvRef parent_ref;
    HashMap *members;
} MXEnv;

void mx_env_init(MXEnv *env, MXEnvRef parent_ref);

void mx_env_declare(MXEnv *env, const char *name, MxirNodeRef decl_ref,
                    AstNodeRef ast_node_ref);

void mx_env_get();

void mx_env_set();

void mx_env_lookup_env_ref();

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
