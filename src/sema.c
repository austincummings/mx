#include "sema.h"
#include "arena.h"
#include "array_list.h"
#include "comptime_value.h"
#include "debug.h"
#include "map.h"
#include "ts_ext.h"
#include <tree_sitter/api.h>

static MXComptimeValue comptime_eval_node(MXSema *sema, TSNode node);

static MXComptimeValueCType resolve_c_type(MXSema *sema, TSNode node) {
    assert(!ts_node_is_null(node));

    HashMap *query_results =
        ts_node_query(&sema->a, node, tree_sitter_mx(),
                      "(comptime_call_expr"
                      "  comptime_arguments: (expr_list ((string_literal) "
                      "@string_literal)))");
    assert(query_results != NULL);

    TSNode arg_node = ts_query_first_node(query_results, "string_literal");
    const char *text = ts_node_text(&sema->a, arg_node, sema->src);

    debug("text = %s\n", text);

    CType c_type = C_TYPE_UNKNOWN;
    if (strcmp(text, "\"int\"") == 0) {
        c_type = C_TYPE_INT;
    } else if (strcmp(text, "\"unsigned int\"") == 0) {
        c_type = C_TYPE_UINT;
    }

    return (MXComptimeValueCType){.type = c_type};
}

static void error(MXSema *sema, const char *msg, TSNode node) {
    assert(sema != NULL);
    assert(msg != NULL);
    assert(!ts_node_is_null(node));
    fprintf(stderr, "SEMANTIC ANALYSIS ERROR: %s\n", msg);
}

static MXComptimeValue comptime_eval_module(MXSema *sema, TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));

    ComptimeEnv *env = comptime_env_new(&sema->a, NULL);
    assert(env != NULL);

    MXComptimeValueList children_values = {0};
    MXComptimeValueModule module = {.children = children_values};

    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(module ["
                                           // "    (struct_decl)"
                                           // "    (fn_decl)"
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

        arraylist_init(&sema->a, &children_values, children->count);

        for (size_t i = 0; i < children->count; i++) {
            TSNode child = children->nodes[i];
            MXComptimeValue child_comptime_value =
                comptime_eval_node(sema, child);
            arraylist_add(&sema->a, &children_values, child_comptime_value);
        }
    }

    debug("Children values size: %d\n", children_values.size);

    return (MXComptimeValue){.kind = MX_COMPTIME_VALUE_MODULE,
                             .module = module};
}

static MXComptimeValue comptime_eval_fn_decl(MXSema *sema, TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    return (MXComptimeValue){.kind = MX_COMPTIME_VALUE_FN_DECL};
}

static MXComptimeValue comptime_eval_struct_decl(MXSema *sema, TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    return (MXComptimeValue){.kind = MX_COMPTIME_VALUE_STRUCT_DECL};
}

static MXComptimeValue comptime_eval_const_decl(MXSema *sema, TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));

    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(const_decl"
                                           "  name: (_) @name"
                                           "  type: (_)? @type"
                                           "  value: (_) @value)");
    assert(query_results != NULL);

    TSNode name_node = ts_query_first_node(query_results, "name");
    assert(!ts_node_is_null(name_node));

    const char *name = ts_node_text(&sema->a, name_node, sema->src);
    debug("static decl: %s\n", name);

    if (ts_query_has_capture(query_results, "type")) {
        TSNode type_node = ts_query_first_node(query_results, "type");
        ts_node_print(&sema->a, type_node, sema->src);
        MXComptimeValue type_value = comptime_eval_node(sema, type_node);
        (void)type_value;
    }

    TSNode value_node = ts_query_first_node(query_results, "value");
    assert(!ts_node_is_null(value_node));

    MXComptimeValue value = comptime_eval_node(sema, value_node);
    debug("static decl value: %d\n", value.kind);

    return (MXComptimeValue){.kind = MX_COMPTIME_VALUE_EMPTY};
}

static MXComptimeValue comptime_eval_expr_stmt(MXSema *sema, TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    return (MXComptimeValue){.kind = MX_COMPTIME_VALUE_EMPTY};
}

static MXComptimeValue comptime_eval_comptime_expr(MXSema *sema, TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(comptime_expr"
                                           "  expr: (_) @expr)");
    assert(query_results != NULL);
    TSNode expr_node = ts_query_first_node(query_results, "expr");
    assert(!ts_node_is_null(expr_node));

    return comptime_eval_node(sema, expr_node);
}

static MXComptimeValue comptime_eval_comptime_call_expr(MXSema *sema,
                                                        TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    HashMap *query_results =
        ts_node_query(&sema->a, node, tree_sitter_mx(),
                      "(comptime_call_expr"
                      "  callee: (_) @callee"
                      "  comptime_arguments: (expr_list (_) @comptime_args))");
    assert(query_results != NULL);
    TSNode callee_node = ts_query_first_node(query_results, "callee");
    assert(!ts_node_is_null(callee_node));

    if (strcmp(ts_node_text(&sema->a, callee_node, sema->src), "c_type") == 0) {
        MXComptimeValueCType c_type = resolve_c_type(sema, node);
        if (c_type.type == C_TYPE_UNKNOWN) {
            error(sema, "Unknown C Type", node);
        }
    }

    if (strcmp(ts_node_text(&sema->a, callee_node, sema->src), "c_value") ==
        0) {
        debug("C_value\n");
    }

    return (MXComptimeValue){.kind = MX_COMPTIME_VALUE_EMPTY};
}

static MXComptimeValue comptime_eval_node(MXSema *sema, TSNode node) {
    const char *node_type = ts_node_type(node);
    if (strcmp(node_type, "module") == 0) {
        return comptime_eval_module(sema, node);
    } else if (strcmp(node_type, "fn_decl") == 0) {
        return comptime_eval_fn_decl(sema, node);
    } else if (strcmp(node_type, "struct_decl") == 0) {
        return comptime_eval_struct_decl(sema, node);
    } else if (strcmp(node_type, "const_decl") == 0) {
        return comptime_eval_const_decl(sema, node);
    } else if (strcmp(node_type, "expr_stmt") == 0) {
        return comptime_eval_expr_stmt(sema, node);
    } else if (strcmp(node_type, "comptime_expr") == 0) {
        return comptime_eval_comptime_expr(sema, node);
    } else if (strcmp(node_type, "comptime_call_expr") == 0) {
        return comptime_eval_comptime_call_expr(sema, node);
    } else {
        ts_node_print(&sema->a, node, sema->src);
        todo("Unhandled node type: %s\n", node_type);
    }

    abort();
}

static MXComptimeValue comptime_eval(MXSema *sema) {
    assert(sema != NULL);

    TSNode root_node = ts_tree_root_node(sema->tree);
    comptime_eval_node(sema, root_node);

    return (MXComptimeValue){0};
}

ComptimeEnv *comptime_env_new(Arena *a, ComptimeEnv *parent) {
    ComptimeEnv *self = arena_alloc_struct(a, ComptimeEnv);
    self->parent = parent;
    self->members = hashmap_init(a);
    return self;
}

MXSema mx_semantic_analyzer_new(const char *src) {
    MXSema self = {0};

    self.a = (Arena){0};
    arena_init(&self.a, ARENA_DEFAULT_RESERVE_SIZE);

    self.src = src;

    return self;
}

void mx_semantic_analyzer_analyze(MXSema *self) {
    assert(self != NULL);
    comptime_eval(self);
}
