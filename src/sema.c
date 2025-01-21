#include "sema.h"
#include "arena.h"
#include "array_list.h"
#include "debug.h"
#include "map.h"
#include "static_value.h"
#include "ts_ext.h"
#include <tree_sitter/api.h>

static MXStaticValue static_eval_node(MXSemanticAnalyzer *sema, TSNode node);

static MXStaticValueCType resolve_c_type(MXSemanticAnalyzer *sema,
                                         TSNode node) {
    assert(!ts_node_is_null(node));

    HashMap *query_results = ts_node_query(
        &sema->a, node, tree_sitter_mx(),
        "(static_call_expr"
        "  static_arguments: (expr_list ((string_literal) @string_literal)))");
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

    return (MXStaticValueCType){.type = c_type};
}

static void error(MXSemanticAnalyzer *sema, const char *msg, TSNode node) {
    assert(sema != NULL);
    assert(msg != NULL);
    assert(!ts_node_is_null(node));
    fprintf(stderr, "SEMANTIC ANALYSIS ERROR: %s\n", msg);
}

static MXStaticValue static_eval_module(MXSemanticAnalyzer *sema, TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));

    StaticEnv *env = static_env_new(&sema->a, NULL);
    assert(env != NULL);

    MXStaticValueList children_values = {0};
    MXStaticValueModule module = {.children = children_values};

    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(module ["
                                           // "    (struct_decl)"
                                           // "    (fn_decl)"
                                           "    (static_decl)"
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
            MXStaticValue child_static_value = static_eval_node(sema, child);
            arraylist_add(&sema->a, &children_values, child_static_value);
        }
    }

    debug("Children values size: %d\n", children_values.size);

    return (MXStaticValue){.kind = MX_STATIC_VALUE_MODULE, .module = module};
}

static MXStaticValue static_eval_fn_decl(MXSemanticAnalyzer *sema,
                                         TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    return (MXStaticValue){.kind = MX_STATIC_VALUE_FN_DECL};
}

static MXStaticValue static_eval_struct_decl(MXSemanticAnalyzer *sema,
                                             TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    return (MXStaticValue){.kind = MX_STATIC_VALUE_STRUCT_DECL};
}

static MXStaticValue static_eval_static_decl(MXSemanticAnalyzer *sema,
                                             TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));

    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(static_decl"
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
        MXStaticValue type_value = static_eval_node(sema, type_node);
        (void)type_value;
    }

    TSNode value_node = ts_query_first_node(query_results, "value");
    assert(!ts_node_is_null(value_node));

    MXStaticValue value = static_eval_node(sema, value_node);
    debug("static decl value: %d\n", value.kind);

    return (MXStaticValue){.kind = MX_STATIC_VALUE_EMPTY};
}

static MXStaticValue static_eval_expr_stmt(MXSemanticAnalyzer *sema,
                                           TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    return (MXStaticValue){.kind = MX_STATIC_VALUE_EMPTY};
}

static MXStaticValue static_eval_static_expr(MXSemanticAnalyzer *sema,
                                             TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    HashMap *query_results = ts_node_query(&sema->a, node, tree_sitter_mx(),
                                           "(static_expr"
                                           "  expr: (_) @expr)");
    assert(query_results != NULL);
    TSNode expr_node = ts_query_first_node(query_results, "expr");
    assert(!ts_node_is_null(expr_node));

    return static_eval_node(sema, expr_node);
}

static MXStaticValue static_eval_static_call_expr(MXSemanticAnalyzer *sema,
                                                  TSNode node) {
    assert(sema != NULL);
    assert(!ts_node_is_null(node));
    HashMap *query_results =
        ts_node_query(&sema->a, node, tree_sitter_mx(),
                      "(static_call_expr"
                      "  callee: (_) @callee"
                      "  static_arguments: (expr_list (_) @static_args))");
    assert(query_results != NULL);
    TSNode callee_node = ts_query_first_node(query_results, "callee");
    assert(!ts_node_is_null(callee_node));

    if (strcmp(ts_node_text(&sema->a, callee_node, sema->src), "c_type") == 0) {
        MXStaticValueCType c_type = resolve_c_type(sema, node);
        if (c_type.type == C_TYPE_UNKNOWN) {
            error(sema, "Unknown C Type", node);
        }
    }

    if (strcmp(ts_node_text(&sema->a, callee_node, sema->src), "c_value") ==
        0) {
        debug("C_value\n");
    }

    return (MXStaticValue){.kind = MX_STATIC_VALUE_EMPTY};
}

static MXStaticValue static_eval_node(MXSemanticAnalyzer *sema, TSNode node) {
    const char *node_type = ts_node_type(node);
    if (strcmp(node_type, "module") == 0) {
        return static_eval_module(sema, node);
    } else if (strcmp(node_type, "fn_decl") == 0) {
        return static_eval_fn_decl(sema, node);
    } else if (strcmp(node_type, "struct_decl") == 0) {
        return static_eval_struct_decl(sema, node);
    } else if (strcmp(node_type, "static_decl") == 0) {
        return static_eval_static_decl(sema, node);
    } else if (strcmp(node_type, "expr_stmt") == 0) {
        return static_eval_expr_stmt(sema, node);
    } else if (strcmp(node_type, "static_expr") == 0) {
        return static_eval_static_expr(sema, node);
    } else if (strcmp(node_type, "static_call_expr") == 0) {
        return static_eval_static_call_expr(sema, node);
    } else {
        ts_node_print(&sema->a, node, sema->src);
        todo("Unhandled node type: %s\n", node_type);
    }

    abort();
}

static MXStaticValue static_eval(MXSemanticAnalyzer *sema) {
    assert(sema != NULL);

    TSNode root_node = ts_tree_root_node(sema->tree);
    static_eval_node(sema, root_node);

    return (MXStaticValue){0};
}

StaticEnv *static_env_new(Arena *a, StaticEnv *parent) {
    StaticEnv *self = arena_alloc_struct(a, StaticEnv);
    self->parent = parent;
    self->members = hashmap_init(a);
    return self;
}

MXSemanticAnalyzer mx_semantic_analyzer_new(const char *src) {
    MXSemanticAnalyzer self = {0};

    self.a = (Arena){0};
    arena_init(&self.a, ARENA_DEFAULT_RESERVE_SIZE);

    self.src = src;

    return self;
}

void mx_semantic_analyzer_analyze(MXSemanticAnalyzer *self) {
    assert(self != NULL);
    static_eval(self);
}
