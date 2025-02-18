#include "sema.h"
#include "array_list.h"
#include "debug.h"
#include "mem.h"
#include "str.h"

static void emit(MXSema *sema, MxirNode node) {
    arraylist_add(sema->permanent_arena, &sema->ir->nodes, node);
}

static void error(MXSema *sema, MXDiagnosticKind kind, MXRange range) {
    MXDiagnostic diag = {.range = range, .kind = kind};
    arraylist_add(sema->permanent_arena, &sema->ast->diagnostics, diag);
}

// static bool is_entry_point(MXSema *sema, AstNodeRef node_ref) {
//     AstNode *node = arraylist_get(&sema->ast->nodes, node_ref);
//     if (strcmp(node->type, "fn_decl") == 0) {
//         // Get the name node
//         AstNodeRef name_ref =
//             (AstNodeRef)hashmap_get(node->named_children, "name");
//         AstNode *name_node = arraylist_get(&sema->ast->nodes, name_ref);
//         if (strcmp(name_node->text, "main") == 0) {
//             return true;
//         }
//     }
//
//     return false;
// }
//
// static void translate_source_file(MXSema *sema, AstNodeRef node_ref) {
//     // Translate the source file. Here we should find the entrypoint function
//     // first and instantiate it, then emit the function.
//     AstNode *node = arraylist_get(&sema->ast->nodes, node_ref);
//     for (size_t i = 0; i < node->children->size; i++) {
//         AstNodeRef child_ref = *arraylist_get(node->children, i);
//         if (is_entry_point(sema, child_ref)) {
//             // Translate the function
//         }
//     }
// }
//
// static void translate_node(MXSema *sema, AstNodeRef node_ref) {
//     // Get the node from the AST
//     AstNode *node = arraylist_get(&sema->ast->nodes, node_ref);
//     const char *type = node->type;
//
//     if (strcmp(type, "source_file") == 0) {
//         translate_source_file(sema, node_ref);
//     } else {
//         // Unhandled node type
//         error(sema, MX_DIAG_UNHANDLED, node->range);
//     }
// }

static MXEnvRef mk_env(MXSema *sema, MXEnvRef parent_ref) {
    MXEnv env = {0};
    env.parent_ref = parent_ref;
    env.members = hashmap_init(sema->permanent_arena);
    arraylist_add(sema->permanent_arena, sema->envs, env);
    return sema->envs->size - 1;
}

static void bind_node(MXSema *sema, AstNodeRef node_ref, MXEnvRef env_ref);

static void bind_source_file(MXSema *sema, AstNodeRef node_ref,
                             MXEnvRef env_ref) {
    debug("Binding source file\n");
    AstNode *node = arraylist_get(&sema->ast->nodes, node_ref);
    for (size_t i = 0; i < node->children->size; i++) {
        AstNodeRef child_ref = *arraylist_get(node->children, i);
        bind_node(sema, child_ref, env_ref);
    }
}

static void bind_fn_decl(MXSema *sema, AstNodeRef node_ref, MXEnvRef env_ref) {
    debug("Binding function declaration\n");
    AstNode *node = arraylist_get(&sema->ast->nodes, node_ref);
    // Get the name node
    AstNodeRef name_ref = (AstNodeRef)hashmap_get(node->named_children, "name");
    AstNode *name_node = arraylist_get(&sema->ast->nodes, name_ref);
    String *name = name_node->text;
    debug("Function name: %s\n", name);
}

static void bind_noop(MXSema *, AstNodeRef, MXEnvRef) {
    // Do nothing
    debug("No-op binding\n");
}

typedef void (*BindFn)(MXSema *, AstNodeRef, MXEnvRef);

static struct {
    const char *type;
    BindFn fn;
} bind_fns[] = {
    {"source_file", bind_source_file},
    {"fn_decl", bind_fn_decl},
    {"line_comment", bind_noop},
};

static void bind_node(MXSema *sema, AstNodeRef node_ref, MXEnvRef env_ref) {
    assert(sema != NULL);

    AstNode *node = arraylist_get(&sema->ast->nodes, node_ref);
    String *type = node->type;

    for (size_t i = 0; i < sizeof(bind_fns) / sizeof(bind_fns[0]); i++) {
        if (string_eq_cstr(type, bind_fns[i].type) == 0) {
            bind_fns[i].fn(sema, node_ref, env_ref);
            return;
        }
    }

    error(sema, MX_DIAG_UNHANDLED_BINDING, node->range);
}

static void bind(MXSema *sema) {
    // Walk the AST
    // - For each declaration, bind it to the current environment
    // - For each scope introduction, create a new environment, and set the
    //   parent to current env For each scope exit, set the current env to the
    //   parent env
    // - For each reference, look up the binding in the current env
    bind_node(sema, 0, sema->root_env_ref);
}

MXSema *mx_sema_new(Arena *permanent_arena) {
    MXSema *sema = arena_alloc_struct(permanent_arena, MXSema);
    sema->permanent_arena = permanent_arena;
    sema->arena = (Arena){0};
    arena_init(&sema->arena, ARENA_DEFAULT_RESERVE_SIZE);
    sema->ast = NULL;
    sema->ir = NULL;
    sema->envs = arena_alloc_struct(permanent_arena, MXEnvList);
    arraylist_init(permanent_arena, sema->envs, 8);
    // Set up the root environment
    MXEnv root_env = {0};
    arraylist_add(permanent_arena, sema->envs, root_env);
    sema->root_env_ref = 0;
    sema->current_env_ref = 0;
    return sema;
}

Mxir *analyze(Arena *permanent_arena, Ast *ast) {
    MXSema *sema = mx_sema_new(permanent_arena);
    assert(sema != NULL);

    sema->ast = ast;
    sema->ir = mxir_new(permanent_arena);

    bind(sema);

    assert(sema->ir != NULL);
    return sema->ir;
}
