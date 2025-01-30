#include "mxir.h"
#include "arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline MXIRNode *mxir_alloc(Arena *arena, MXIRKind kind) {
    MXIRNode *node = arena_alloc_struct(arena, MXIRNode);
    node->kind = kind;
    return node;
}

MXIRNodeList mxir_node_list(Arena *arena) {
    MXIRNodeList list = {0};
    arraylist_init(arena, &list, 16);
    return list;
}

MXIRNode *mxir_module(Arena *arena, MXIRNodeList fns) {
    MXIRNode *node = mxir_alloc(arena, MXIR_MODULE);
    node->module.fns = fns;
    return node;
}

MXIRNode *mxir_fn(Arena *arena, const char *name, MXIRNodeList params,
                  MXIRNode *body) {
    MXIRNode *node = mxir_alloc(arena, MXIR_FN);
    node->fn.name = name;
    node->fn.params = params;
    node->fn.body = body;
    return node;
}

MXIRNode *mxir_var_decl(Arena *arena, const char *name, MXIRNode *value) {
    MXIRNode *node = mxir_alloc(arena, MXIR_VAR_DECL);
    node->var_decl.name = name;
    node->var_decl.value = value;
    return node;
}

MXIRNode *mxir_if(Arena *arena, MXIRNode *condition, MXIRNode *then_block,
                  MXIRNode *else_block) {
    MXIRNode *node = mxir_alloc(arena, MXIR_IF);
    node->if_stmt.condition = condition;
    node->if_stmt.then = then_block;
    node->if_stmt.else_ = else_block;
    return node;
}

MXIRNode *mxir_loop(Arena *arena, MXIRNode *body) {
    MXIRNode *node = mxir_alloc(arena, MXIR_LOOP);
    node->loop.body = body;
    return node;
}

MXIRNode *mxir_break(Arena *arena) {
    MXIRNode *node = mxir_alloc(arena, MXIR_BREAK);
    return node;
}

MXIRNode *mxir_continue(Arena *arena) {
    MXIRNode *node = mxir_alloc(arena, MXIR_CONTINUE);
    return node;
}

MXIRNode *mxir_return(Arena *arena, MXIRNode *value) {
    MXIRNode *node = mxir_alloc(arena, MXIR_RETURN);
    node->return_stmt.value = value;
    return node;
}

MXIRNode *mxir_expr_stmt(Arena *arena, MXIRNode *expr) {
    MXIRNode *node = mxir_alloc(arena, MXIR_EXPR_STMT);
    node->expr_stmt.expr = expr;
    return node;
}

MXIRNode *mxir_assign(Arena *arena, MXIRNode *lhs, MXIRNode *value) {
    MXIRNode *node = mxir_alloc(arena, MXIR_ASSIGN);
    node->assign.lhs = lhs;
    node->assign.value = value;
    return node;
}

MXIRNode *mxir_block(Arena *arena, MXIRNodeList stmts) {
    MXIRNode *node = mxir_alloc(arena, MXIR_BLOCK);
    node->block.stmts = stmts;
    return node;
}

MXIRNode *mxir_call(Arena *arena, MXIRNode *callee, MXIRNodeList args) {
    MXIRNode *node = mxir_alloc(arena, MXIR_CALL);
    node->call.callee = callee;
    node->call.args = args;
    return node;
}

MXIRNode *mxir_comptime_int(Arena *arena, uint64_t value) {
    MXIRNode *node = mxir_alloc(arena, MXIR_COMPTIME_INT);
    node->comptime_int.value = value;
    return node;
}

MXIRNode *mxir_comptime_float(Arena *arena, double value) {
    MXIRNode *node = mxir_alloc(arena, MXIR_COMPTIME_FLOAT);
    node->comptime_float.value = value;
    return node;
}

MXIRNode *mxir_comptime_string(Arena *arena, const char *value) {
    MXIRNode *node = mxir_alloc(arena, MXIR_COMPTIME_STRING);
    node->comptime_string.value = value;
    return node;
}

MXIRNode *mxir_comptime_bool(Arena *arena, bool value) {
    MXIRNode *node = mxir_alloc(arena, MXIR_COMPTIME_BOOL);
    node->comptime_bool.value = value;
    return node;
}

// static const char *mxir_kind_to_str(MXIRKind kind) {
//     switch (kind) {
//     case MXIR_MODULE:
//         return "module";
//     case MXIR_FN:
//         return "fn";
//     case MXIR_VAR_DECL:
//         return "var_decl";
//     case MXIR_IF:
//         return "if";
//     case MXIR_LOOP:
//         return "loop";
//     case MXIR_BREAK:
//         return "break";
//     case MXIR_CONTINUE:
//         return "continue";
//     case MXIR_RETURN:
//         return "return";
//     case MXIR_EXPR_STMT:
//         return "expr_stmt";
//     case MXIR_ASSIGN:
//         return "assign";
//     case MXIR_BLOCK:
//         return "block";
//     case MXIR_CALL:
//         return "call";
//     case MXIR_COMPTIME_INT:
//         return "comptime_int";
//     case MXIR_COMPTIME_FLOAT:
//         return "comptime_float";
//     case MXIR_COMPTIME_STRING:
//         return "comptime_string";
//     case MXIR_COMPTIME_BOOL:
//         return "comptime_bool";
//     default:
//         return "unknown";
//     }
// }

static char *append_str(Arena *a, const char *str) {
    size_t len = strlen(str);
    char *dest = (char *)arena_alloc(a, len + 1);
    memcpy(dest, str, len);
    dest[len] = '\0';
    return dest;
}

static const char *mxir_node_to_sexpr_internal(Arena *a, MXIRNode *node) {
    if (!node)
        return append_str(a, "<nil>");

    switch (node->kind) {
    case MXIR_MODULE: {
        char *sexpr = append_str(a, "(module ");
        for (size_t i = 0; i < node->module.fns.size; i++) {
            const char *child =
                mxir_node_to_sexpr_internal(a, &node->module.fns.data[i]);
            sexpr = arena_strcat(a, sexpr, (char *)child);
            sexpr = arena_strcat(a, sexpr, " ");
        }
        sexpr = arena_strcat(a, sexpr, ")");
        return sexpr;
    }
    case MXIR_FN: {
        char *sexpr = arena_strcat(a, "(fn ", (char *)node->fn.name);
        sexpr = arena_strcat(a, sexpr, " ");
        sexpr = arena_strcat(a, sexpr, "(");
        for (size_t i = 0; i < node->fn.params.size; i++) {
            const char *child =
                mxir_node_to_sexpr_internal(a, &node->fn.params.data[i]);
            sexpr = arena_strcat(a, sexpr, (char *)child);
            if (i < node->fn.params.size - 1) {
                sexpr = arena_strcat(a, sexpr, " ");
            }
        }
        sexpr = arena_strcat(a, sexpr, ") ");
        // TODO: return type
        sexpr = arena_strcat(
            a, sexpr, (char *)mxir_node_to_sexpr_internal(a, node->fn.body));
        sexpr = arena_strcat(a, sexpr, ")");
        return sexpr;
    }
    case MXIR_VAR_DECL: {
        char *sexpr =
            arena_strcat(a, "(var_decl ", (char *)node->var_decl.name);
        // TODO: type
        if (node->var_decl.value) {
            sexpr = arena_strcat(a, sexpr, " = ");
            sexpr = arena_strcat(
                a, sexpr,
                (char *)mxir_node_to_sexpr_internal(a, node->var_decl.value));
        }
        sexpr = arena_strcat(a, sexpr, ")");
        return sexpr;
    }
    case MXIR_IF: {
        char *sexpr = append_str(a, "(if ");
        sexpr = arena_strcat(
            a, sexpr,
            (char *)mxir_node_to_sexpr_internal(a, node->if_stmt.condition));
        sexpr = arena_strcat(a, sexpr, " ");
        sexpr = arena_strcat(
            a, sexpr,
            (char *)mxir_node_to_sexpr_internal(a, node->if_stmt.then));
        if (node->if_stmt.else_) {
            sexpr = arena_strcat(a, sexpr, " ");
            sexpr = arena_strcat(
                a, sexpr,
                (char *)mxir_node_to_sexpr_internal(a, node->if_stmt.else_));
        }
        sexpr = arena_strcat(a, sexpr, ")");
        return sexpr;
    }
    case MXIR_BREAK: {
        return append_str(a, "(break)");
    }
    case MXIR_CONTINUE: {
        return append_str(a, "(continue)");
    }
    case MXIR_RETURN: {
        char *sexpr = append_str(a, "(return");
        if (node->return_stmt.value) {
            sexpr = arena_strcat(a, sexpr, " ");
            sexpr = arena_strcat(a, sexpr,
                                 (char *)mxir_node_to_sexpr_internal(
                                     a, node->return_stmt.value));
        }
        sexpr = arena_strcat(a, sexpr, ")");
        return sexpr;
    }
    case MXIR_EXPR_STMT: {
        return mxir_node_to_sexpr_internal(a, node->expr_stmt.expr);
    }
    case MXIR_ASSIGN: {
        char *sexpr = append_str(a, "(assign ");
        sexpr = arena_strcat(
            a, sexpr, (char *)mxir_node_to_sexpr_internal(a, node->assign.lhs));
        sexpr = arena_strcat(a, sexpr, " ");
        sexpr = arena_strcat(
            a, sexpr,
            (char *)mxir_node_to_sexpr_internal(a, node->assign.value));
        sexpr = arena_strcat(a, sexpr, ")");
        return sexpr;
    }
    case MXIR_BLOCK: {
        char *sexpr = append_str(a, "(block ");
        for (size_t i = 0; i < node->block.stmts.size; i++) {
            const char *child =
                mxir_node_to_sexpr_internal(a, &node->block.stmts.data[i]);
            sexpr = arena_strcat(a, sexpr, (char *)child);
            sexpr = arena_strcat(a, sexpr, " ");
        }
        return sexpr;
    }
    case MXIR_CALL: {
        char *sexpr = append_str(a, "(call ");
        sexpr = arena_strcat(
            a, sexpr,
            (char *)mxir_node_to_sexpr_internal(a, node->call.callee));
        sexpr = arena_strcat(a, sexpr, ")");
        return sexpr;
    }
    case MXIR_COMPTIME_INT: {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "(comptime_int %lu)",
                 node->comptime_int.value);
        return append_str(a, buffer);
    }
    case MXIR_COMPTIME_FLOAT: {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "(comptime_float %f)",
                 node->comptime_float.value);
        return append_str(a, buffer);
    }
    case MXIR_COMPTIME_STRING: {
        return arena_strcat(
            a, "(comptime_string \"",
            arena_strcat(a, (char *)node->comptime_string.value, "\")"));
    }
    case MXIR_COMPTIME_BOOL: {
        return append_str(a, node->comptime_bool.value
                                 ? "(comptime_bool true)"
                                 : "(comptime_bool false)");
    }
    default:
        return append_str(a, "(unknown)");
    }
}

const char *mxir_node_to_sexpr(Arena *a, MXIRNode *node) {
    return mxir_node_to_sexpr_internal(a, node);
}

const char *mxir_node_list_to_sexpr(Arena *a, MXIRNodeList *node_list) {
    if (!node_list || node_list->size == 0) {
        return append_str(a, "()");
    }

    char *sexpr = append_str(a, "(");
    for (size_t i = 0; i < node_list->size; i++) {
        const char *child = mxir_node_to_sexpr_internal(a, &node_list->data[i]);
        sexpr = arena_strcat(a, sexpr, (char *)child);
        if (i < node_list->size - 1) {
            sexpr = arena_strcat(a, sexpr, " ");
        }
    }
    sexpr = arena_strcat(a, sexpr, ")");
    return sexpr;
}
