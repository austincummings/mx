#include "sema.h"
#include "arena.h"
#include "array_list.h"
#include "comptime_value.h"
#include "debug.h"
#include "map.h"
#include "mxir.h"
#include "ts_ext.h"
#include <tree_sitter/api.h>

static void bind_node(MXSema *sema, TSNode node, MXComptimeEnv *env);

// static MXComptimeValueCType resolve_c_type(MXSema *sema, TSNode node,
// MXComptimeEnv *env) {
//     assert(!ts_node_is_null(node));
//
//     HashMap *query_results =
//         query(&sema->a, node,
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

    uint32_t line = ts_node_start_point(node).row;
    uint32_t col = ts_node_start_point(node).column;
    fprintf(stderr, "SEMANTIC ERROR [Line %u, Col %u]: %s\n", line, col, msg);

    const char *node_text = ts_node_line_text(&sema->a, node, sema->src);
    fprintf(stderr, "  %s\n", node_text);
}

static MXComptimeEnv *open_env(MXSema *sema, MXComptimeEnv *parent,
                               TSNode node) {
    assert(sema != NULL);
    MXComptimeEnv *env = mx_comptime_env_new(&sema->a, parent, node);
    assert(env != NULL);
    arraylist_add(&sema->a, &sema->envs, env);
    return env;
}

static void bind_module(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env == NULL); // env should be null for the module node

    env = open_env(sema, NULL, node);
    assert(env != NULL);

    HashMap *query_results =
        query(&sema->a, node, "(module (_) @children)", true);
    assert(query_results != NULL);

    if (ts_query_has_capture(query_results, "children")) {
        TSNodeList *children = ts_query_nodes(query_results, "children");

        for (size_t i = 0; i < children->size; i++) {
            TSNode child = children->data[i];
            bind_node(sema, child, env);
        }
    }

    mx_comptime_env_dump(env, 0, true, sema->src);

    env = NULL;
}

static void bind_fn_decl(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results =
        query(&sema->a, node,
              "(fn_decl"
              "  name: (_) @name"
              "  comptime_parameters: (_)? @comptime_params"
              "  parameters: (_)? @params"
              "  return_type: (_) @return_type"
              "  body: (_) @body)",
              true);
    assert(query_results != NULL);

    TSNode name_node = ts_query_first_node(query_results, "name");
    assert(!ts_node_is_null(name_node));

    const char *name = ts_node_text(&sema->a, name_node, sema->src);

    if (!mx_comptime_env_declare(env, name, node)) {
        error(sema, "Name already declared", name_node);
        return;
    }

    // Introduce a new environment for the comptime function params
    env = open_env(sema, env, node);

    // Bind the comptime parameters
    if (ts_query_has_capture(query_results, "comptime_params")) {
        TSNode comptime_params_node =
            ts_query_first_node(query_results, "comptime_params");

        HashMap *query_results =
            query(&sema->a, comptime_params_node,
                  "(parameter_list (_) @parameters)", true);
        assert(query_results != NULL);

        if (ts_query_has_capture(query_results, "parameters")) {
            TSNodeList *comptime_params_list =
                ts_query_nodes(query_results, "parameters");

            for (size_t i = 0; i < comptime_params_list->size; i++) {
                TSNode parameter = comptime_params_list->data[i];
                bind_node(sema, parameter, env);
            }
        }
    }

    // Bind the parameters
    if (ts_query_has_capture(query_results, "params")) {
        TSNode params_node = ts_query_first_node(query_results, "params");

        HashMap *query_results = query(
            &sema->a, params_node, "(parameter_list (_) @parameters)", true);
        assert(query_results != NULL);

        if (ts_query_has_capture(query_results, "parameters")) {
            TSNodeList *params_list =
                ts_query_nodes(query_results, "parameters");

            for (size_t i = 0; i < params_list->size; i++) {
                TSNode parameter = params_list->data[i];
                bind_node(sema, parameter, env);
            }
        }
    }

    // Bind the return type
    TSNode return_type_node = ts_query_first_node(query_results, "return_type");
    bind_node(sema, return_type_node, env);

    // Bind the body
    TSNode body_node = ts_query_first_node(query_results, "body");
    if (!ts_node_is_null(body_node)) {
        bind_node(sema, body_node, env);
    }

    // Restore the parent environment
    env = env->parent;
}

static void bind_var_decl(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results = query(&sema->a, node,
                                   "(var_decl"
                                   "  name: (_) @name"
                                   "  type: (_)? @type"
                                   "  value: (_) @value)",
                                   true);
    assert(query_results != NULL);

    TSNode name_node = ts_query_first_node(query_results, "name");
    assert(!ts_node_is_null(name_node));

    const char *name = ts_node_text(&sema->a, name_node, sema->src);

    if (!mx_comptime_env_declare(env, name, node)) {
        error(sema, "Variable name already declared", name_node);
    }
}

static void bind_const_decl(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results = query(&sema->a, node,
                                   "(const_decl"
                                   "  name: (_) @name"
                                   "  type: (_)? @type"
                                   "  value: (_) @value)",
                                   true);
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

    HashMap *query_results = query(&sema->a, node,
                                   "(expr_stmt"
                                   "  expr: (_) @expr)",
                                   true);
    assert(query_results != NULL);

    TSNode expr_node = ts_query_first_node(query_results, "expr");
    assert(!ts_node_is_null(expr_node));
    bind_node(sema, expr_node, env);
}

static void bind_comptime_expr(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results = query(&sema->a, node,
                                   "(comptime_expr"
                                   "  expr: (_) @expr)",
                                   true);
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
        query(&sema->a, node,
              "(comptime_call_expr"
              "  callee: (_) @callee"
              "  comptime_arguments: (expr_list (_) @comptime_args))",
              true);
    assert(query_results != NULL);
    TSNode callee_node = ts_query_first_node(query_results, "callee");
    assert(!ts_node_is_null(callee_node));

    if (strcmp(ts_node_text(&sema->a, callee_node, sema->src), "c_type") == 0) {
        todo("fix this");
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
    env = open_env(sema, env, node);

    HashMap *query_results =
        query(&sema->a, node, "(block (_) @children)", true);
    assert(query_results != NULL);

    if (ts_query_has_capture(query_results, "children")) {
        TSNodeList *children = ts_query_nodes(query_results, "children");

        for (size_t i = 0; i < children->size; i++) {
            TSNode child = children->data[i];
            bind_node(sema, child, env);
        }
    }

    mx_comptime_env_dump(env, 0, true, sema->src);

    // Restore the parent environment
    env = env->parent;
}

static void bind_return_stmt(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results = query(&sema->a, node,
                                   "(return_stmt"
                                   "  expr: (_) @expr)",
                                   true);
    assert(query_results != NULL);

    TSNode expr_node = ts_query_first_node(query_results, "expr");
    if (!ts_node_is_null(expr_node)) {
        bind_node(sema, expr_node, env);
    }
}

static void bind_if_stmt(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results = query(&sema->a, node,
                                   "(if_stmt"
                                   "  condition: (_) @condition"
                                   "  then: (_) @then"
                                   "  else: (_)? @else)",
                                   true);
    assert(query_results != NULL);

    TSNode condition_node = ts_query_first_node(query_results, "condition");
    assert(!ts_node_is_null(condition_node));
    bind_node(sema, condition_node, env);

    TSNode then_node = ts_query_first_node(query_results, "then");
    assert(!ts_node_is_null(then_node));
    bind_node(sema, then_node, env);

    if (ts_query_has_capture(query_results, "else")) {
        TSNode else_node = ts_query_first_node(query_results, "else");
        assert(!ts_node_is_null(else_node));
        bind_node(sema, else_node, env);
    }
}

static void bind_parameter(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    assert(env != NULL);

    HashMap *query_results = query(&sema->a, node,
                                   "(parameter"
                                   "  name: (_) @name"
                                   "  type: (_) @type)",
                                   true);
    assert(query_results != NULL);

    TSNode name_node = ts_query_first_node(query_results, "name");
    assert(!ts_node_is_null(name_node));

    const char *name = ts_node_text(&sema->a, name_node, sema->src);

    if (!mx_comptime_env_declare(env, name, node)) {
        error(sema, "Parameter name already declared", name_node);
    }
}

static void bind_noop(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    (void)sema;
    (void)node;
    (void)env;
}

typedef void (*SemaFn)(MXSema *, TSNode, MXComptimeEnv *);

static struct {
    const char *type;
    SemaFn fn;
} bind_fns[] = {
    {"module", bind_module},
    {"fn_decl", bind_fn_decl},
    {"var_decl", bind_var_decl},
    {"const_decl", bind_const_decl},
    {"expr_stmt", bind_expr_stmt},
    {"comptime_expr", bind_comptime_expr},
    {"comptime_call_expr", bind_comptime_call_expr},
    {"block", bind_block},
    {"return_stmt", bind_return_stmt},
    {"if_stmt", bind_if_stmt},
    {"parameter", bind_parameter},
    {"int_literal", bind_noop},
    {"line_comment", bind_noop},
    {"identifier", bind_noop},
    {"string_literal", bind_noop},
    {"bool_literal", bind_noop},
};

static void bind_node(MXSema *sema, TSNode node, MXComptimeEnv *env) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));

    const char *node_type = ts_node_type(node);

    for (size_t i = 0; i < sizeof(bind_fns) / sizeof(bind_fns[0]); i++) {
        if (strcmp(node_type, bind_fns[i].type) == 0) {
            bind_fns[i].fn(sema, node, env);
            return;
        }
    }

    ts_node_print(&sema->a, node, sema->src);
    todo("Unhandled node type: %s\n", node_type);
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

    MXIRNodeList fns = mxir_node_list(&self->a);

    MXIRNode *comptime_int = mxir_comptime_int(&self->a, 42);

    MXIRNode *return_stmt = mxir_return(&self->a, comptime_int);

    MXIRNodeList body_stmts = mxir_node_list(&self->a);
    arraylist_add(&self->a, &body_stmts, *return_stmt);

    MXIRNode *fn_body = mxir_block(&self->a, body_stmts);

    MXIRNode *fn = mxir_fn(&self->a, "main", mxir_node_list(&self->a), fn_body);
    arraylist_add(&self->a, &fns, *fn);

    const char *sexpr = mxir_node_to_sexpr(&self->a, fn);
    debug("%s\n", sexpr);

    bind(self);
}
