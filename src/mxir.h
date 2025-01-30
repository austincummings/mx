#ifndef MXIR_H
#define MXIR_H

#include "array_list.h"

typedef enum {
    // Declarations
    MXIR_FN,
    MXIR_VAR_DECL,

    // Statements
    MXIR_IF,
    MXIR_LOOP,
    MXIR_BREAK,
    MXIR_CONTINUE,
    MXIR_RETURN,
    MXIR_EXPR_STMT,
    MXIR_ASSIGN,

    // Expressions
    MXIR_BLOCK,
    MXIR_CALL,
    MXIR_COMPTIME_INT,
    MXIR_COMPTIME_FLOAT,
    MXIR_COMPTIME_STRING,
    MXIR_COMPTIME_BOOL
} MXIRKind;

typedef struct MXIRNode MXIRNode;

typedef ArrayList(MXIRNode) MXIRNodeList;

typedef struct {
    const char *name;
    MXIRNodeList params;
    // TODO: return type
    MXIRNode *body;
} MXIRFn;

typedef struct {
    const char *name;
    // TODO: type
    MXIRNode *value;
} MXIRVarDecl;

// Statements

typedef struct {
    MXIRNode *condition;
    MXIRNode *then;
    MXIRNode *else_;
} MXIRIf;

typedef struct {
    MXIRNode *body;
} MXIRLoop;

typedef struct {
    uint8_t _;
} MXIRBreak;

typedef struct {
    uint8_t _;
} MXIRContinue;

typedef struct {
    MXIRNode *value;
} MXIRReturn;

typedef struct {
    MXIRNode *expr;
} MXIRExprStmt;

typedef struct {
    MXIRNode *lhs;
    MXIRNode *value;
} MXIRAssign;

// Expressions

typedef struct {
    MXIRNodeList stmts;
} MXIRBlock;

typedef struct {
    MXIRNode *callee;
    MXIRNodeList args;
} MXIRCall;

typedef struct {
    uint64_t value;
} MXIRComptimeInt;

typedef struct {
    double value;
} MXIRComptimeFloat;

typedef struct {
    const char *value;
} MXIRComptimeString;

typedef struct {
    bool value;
} MXIRComptimeBool;

struct MXIRNode {
    MXIRKind kind;
    union {
        MXIRFn fn;
        MXIRVarDecl var_decl;
        MXIRIf if_stmt;
        MXIRLoop loop;
        MXIRBreak break_stmt;
        MXIRContinue continue_stmt;
        MXIRReturn return_stmt;
        MXIRExprStmt expr_stmt;
        MXIRAssign assign;
        MXIRBlock block;
        MXIRCall call;
        MXIRComptimeInt comptime_int;
        MXIRComptimeFloat comptime_float;
        MXIRComptimeString comptime_string;
        MXIRComptimeBool comptime_bool;
    };
};

MXIRNodeList mxir_node_list(Arena *arena);
MXIRNode *mxir_fn(Arena *arena, const char *name, MXIRNodeList params,
                  MXIRNode *body);
MXIRNode *mxir_var_decl(Arena *arena, const char *name, MXIRNode *value);
MXIRNode *mxir_if(Arena *arena, MXIRNode *condition, MXIRNode *then_block,
                  MXIRNode *else_block);
MXIRNode *mxir_loop(Arena *arena, MXIRNode *body);
MXIRNode *mxir_break(Arena *arena);
MXIRNode *mxir_continue(Arena *arena);
MXIRNode *mxir_return(Arena *arena, MXIRNode *value);
MXIRNode *mxir_expr_stmt(Arena *arena, MXIRNode *expr);
MXIRNode *mxir_assign(Arena *arena, MXIRNode *lhs, MXIRNode *value);
MXIRNode *mxir_block(Arena *arena, MXIRNodeList stmts);
MXIRNode *mxir_call(Arena *arena, MXIRNode *callee, MXIRNodeList args);
MXIRNode *mxir_comptime_int(Arena *arena, uint64_t value);
MXIRNode *mxir_comptime_float(Arena *arena, double value);
MXIRNode *mxir_comptime_string(Arena *arena, const char *value);
MXIRNode *mxir_comptime_bool(Arena *arena, bool value);

const char *mxir_node_to_sexpr(Arena *a, MXIRNode *node);
const char *mxir_node_list_to_sexpr(Arena *a, MXIRNodeList *node_list);

#endif // MXIR_H
