#include "sema.h"
#include "arena.h"
#include "array_list.h"
#include "comptime_value.h"
#include "debug.h"
#include "map.h"
#include "ts_ext.h"
#include <tree_sitter/api.h>

static void bind_node(MXSema *sema, TSNode node, MXComptimeEnv *env);

// static MXComptimeValueCType resolve_c_type(MXSema *sema, TSNode node,
// MXComptimeEnv *env) {
//     assert(!ts_node_is_null(node));
//
//     HashMap *query_results =
//         ts_node_query(&sema->a, node, tree_sitter_mx(),
//                       "(comptime_call_expr"
//                       "  comptime_arguments: (expr_list ((string_literal) "
//                       "@string_literal)))");
//     assert(query_results != NULL);
//
//     TSNode arg_node = ts_query_first_node(query_results, "string_literal");
//     const char *text = ts_node_text(&sema->a, arg_node, sema->src);
//
//     debug("text = %s\n", text);
//
//     CType c_type = C_TYPE_UNKNOWN;
//     if (strcmp(text, "\"int\"") == 0) {
//         c_type = C_TYPE_INT;
//     } else if (strcmp(text, "\"unsigned int\"") == 0) {
//         c_type = C_TYPE_UINT;
//     }
//
//     return (MXComptimeValueCType){.type = c_type};
// }

static void error(MXSema *sema, const char *msg, TSNode node) {
    assert(sema != NULL);
    assert(msg != NULL);
    assert(!ts_node_is_null(node));
    fprintf(stderr, "SEMANTIC ANALYSIS ERROR: %s\n", msg);
    const char *node_text = ts_node_line_text(&sema->a, node, sema->src);
    fprintf(stderr, "  %s\n", node_text);
    // Print a caret under the node that is in error in red
    // fprintf(stderr, "\033[31m");
    // fprintf(stderr, "\033[0m\n");
}

static MXComptimeEnv *open_env(MXSema *sema, MXComptimeEnv *parent) {
    assert(sema != NULL);
    MXComptimeEnv *env = mx_comptime_env_new(&sema->a, parent);
    assert(env != NULL);
    arraylist_add(&sema->a, &sema->envs, env);
    return env;
}

static void bind_module(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env == NULL); // env should be null for the module node

    env = open_env(sema, NULL);
    assert(env != NULL);

    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(module ["
                                           // "    (struct_decl)"
                                           "    (fn_decl)"
                                           "    (const_decl)"
                                           // "    (var_decl)"
                                           // "    (expr_stmt)"
                                           // "    (break_stmt)"
                                           // "    (continue_stmt)"
                                           // "    (return_stmt)"
                                           // "    (if_stmt)"
                                           // "    (loop_stmt)"
                                           // "    (for_stmt)"
                                           // "    (assign_stmt)"
                                           "] @children)");
    if (ts_query_has_capture(query_results, "children")) {
        TSNodeList *children = ts_query_nodes(query_results, "children");

        for (size_t i = 0; i < children->count; i++) {
            TSNode child = children->nodes[i];
            bind_node(sema, child, env);
        }
    }

    mx_comptime_env_dump(env, 0, true);
}

static void bind_fn_decl(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results =
        ts_node_query(&sema->a, node, tree_sitter_mx(),
                      "(fn_decl"
                      "  name: (_) @name"
                      "  comptime_parameters: (_)? @comptime_params"
                      "  parameters: (_)? @params"
                      "  body: (_) @body)");
    assert(query_results != NULL);

    TSNode name_node = ts_query_first_node(query_results, "name");
    assert(!ts_node_is_null(name_node));

    const char *name = ts_node_text(&sema->a, name_node, sema->src);

    if (!mx_comptime_env_declare(env, name, node)) {
        error(sema, "Name already declared", name_node);
    }

    // Introduce a new environment for the function params
    MXComptimeEnv *fn_env = open_env(sema, env);
    assert(fn_env != NULL);

    // Bind the comptime parameters
    if (ts_query_has_capture(query_results, "comptime_params")) {
        todo("comptime parameters\n");
    }

    // Bind the parameters
    if (ts_query_has_capture(query_results, "params")) {
        todo("parameters\n");
    }

    // Bind the body
    TSNode body_node = ts_query_first_node(query_results, "body");
    if (!ts_node_is_null(body_node)) {
        bind_node(sema, body_node, fn_env);
    }
}

static void bind_var_decl(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(var_decl"
                                           "  name: (_) @name"
                                           "  type: (_)? @type"
                                           "  value: (_) @value)");
    assert(query_results != NULL);

    TSNode name_node = ts_query_first_node(query_results, "name");
    assert(!ts_node_is_null(name_node));

    const char *name = ts_node_text(&sema->a, name_node, sema->src);

    if (!mx_comptime_env_declare(env, name, node)) {
        error(sema, "Variable name already declared", name_node);
    }
}

static void bind_struct_decl(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);
}

static void bind_const_decl(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(const_decl"
                                           "  name: (_) @name"
                                           "  type: (_)? @type"
                                           "  value: (_) @value)");
    assert(query_results != NULL);

    TSNode name_node = ts_query_first_node(query_results, "name");
    assert(!ts_node_is_null(name_node));

    const char *name = ts_node_text(&sema->a, name_node, sema->src);

    if (!mx_comptime_env_declare(env, name, node)) {
        error(sema, "Function name already declared", name_node);
    }
}

static void bind_expr_stmt(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);
}

static void bind_comptime_expr(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(comptime_expr"
                                           "  expr: (_) @expr)");
    assert(query_results != NULL);
    TSNode expr_node = ts_query_first_node(query_results, "expr");
    assert(!ts_node_is_null(expr_node));

    bind_node(sema, expr_node, env);
}

static void bind_comptime_call_expr(MXSema *sema, TSNode node,
                                    MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results =
        ts_node_query(&sema->a, node, tree_sitter_mx(),
                      "(comptime_call_expr"
                      "  callee: (_) @callee"
                      "  comptime_arguments: (expr_list (_) @comptime_args))");
    assert(query_results != NULL);
    TSNode callee_node = ts_query_first_node(query_results, "callee");
    assert(!ts_node_is_null(callee_node));

    if (strcmp(ts_node_text(&sema->a, callee_node, sema->src), "c_type") == 0) {
        todo("fix this");
        // voidCType c_type = resolve_c_type(sema, node);
        // if (c_type.type == C_TYPE_UNKNOWN) {
        //     error(sema, "Unknown C Type", node);
        // }
    }

    if (strcmp(ts_node_text(&sema->a, callee_node, sema->src), "c_value") ==
        0) {
        debug("C_value\n");
    }
}

static void bind_block(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    // Introduce a new environment for the block
    MXComptimeEnv *block_env = mx_comptime_env_new(&sema->a, env);

    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(block (_)* @children)");
    assert(query_results != NULL);

    if (ts_query_has_capture(query_results, "children")) {
        TSNodeList *children = ts_query_nodes(query_results, "children");

        for (size_t i = 0; i < children->count; i++) {
            TSNode child = children->nodes[i];
            bind_node(sema, child, block_env);
        }
    }

    mx_comptime_env_dump(block_env, 0, true);
}

static void bind_return_stmt(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(return_stmt"
                                           "  expr: (_) @expr)");
    assert(query_results != NULL);

    TSNode expr_node = ts_query_first_node(query_results, "expr");
    if (!ts_node_is_null(expr_node)) {
        bind_node(sema, expr_node, env);
    }
}

static void bind_node(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    // env can be null for the module node so we don't assert it here

    const char *node_type = ts_node_type(node);
    if (strcmp(node_type, "module") == 0) {
        bind_module(sema, node, env);
        return;
    } else if (strcmp(node_type, "fn_decl") == 0) {
        bind_fn_decl(sema, node, env);
        return;
    } else if (strcmp(node_type, "var_decl") == 0) {
        bind_var_decl(sema, node, env);
        return;
    } else if (strcmp(node_type, "struct_decl") == 0) {
        bind_struct_decl(sema, node, env);
        return;
    } else if (strcmp(node_type, "const_decl") == 0) {
        bind_const_decl(sema, node, env);
        return;
    } else if (strcmp(node_type, "expr_stmt") == 0) {
        bind_expr_stmt(sema, node, env);
        return;
    } else if (strcmp(node_type, "comptime_expr") == 0) {
        bind_comptime_expr(sema, node, env);
        return;
    } else if (strcmp(node_type, "comptime_call_expr") == 0) {
        bind_comptime_call_expr(sema, node, env);
        return;
    } else if (strcmp(node_type, "block") == 0) {
        bind_block(sema, node, env);
        return;
    } else if (strcmp(node_type, "return_stmt") == 0) {
        bind_return_stmt(sema, node, env);
        return;
    } else if (strcmp(node_type, "identifier") == 0 ||
               strcmp(node_type, "int_literal") == 0 ||
               strcmp(node_type, "bool_literal") == 0 ||
               strcmp(node_type, "string_literal") == 0) {
        // noop since none of these nodes introduce new bindings
        return;
    } else {
        ts_node_print(&sema->a, node, sema->src);
        todo("Unhandled node type: %s\n", node_type);
    }

    debug("unreachable in bind_node\n");
    abort();
}

static void bind(MXSema *sema) {
    assert(sema != NULL);

    TSNode root_node = ts_tree_root_node(sema->tree);
    bind_node(sema, root_node, NULL);
}

MXSema mx_sema_new(const char *src) {
    MXSema self = {0};

    self.a = (Arena){0};
    arena_init(&self.a, ARENA_DEFAULT_RESERVE_SIZE);

    self.envs = (MXComptimeEnvRefList){0};
    arraylist_init(&self.a, &self.envs, 64);

    self.src = src;

    return self;
}

void mx_sema_analyze(MXSema *self) {
    assert(self != NULL);
    bind(self);
}
