#ifndef COMPTIME_VALUE_H
#define COMPTIME_VALUE_H

#include "array_list.h"
#include "map.h"
#include <tree_sitter/api.h>

typedef enum {
    C_TYPE_UNKNOWN,
    C_TYPE_VOID,
    C_TYPE_BOOL,
    C_TYPE_INT,
    C_TYPE_INT8,
    C_TYPE_INT16,
    C_TYPE_INT32,
    C_TYPE_INT64,
    C_TYPE_UINT,
    C_TYPE_UINT8,
    C_TYPE_UINT16,
    C_TYPE_UINT32,
    C_TYPE_UINT64,
} CType;

typedef enum {
    MX_COMPTIME_VALUE_UNDEFINED,
    // Code Comptime Values
    // Declarations
    MX_COMPTIME_VALUE_MODULE,      // <list of stmts and decls>
    MX_COMPTIME_VALUE_FN_DECL,     // fn f(): comptime_expr {}
    MX_COMPTIME_VALUE_VAR_DECL,    // var my_var = expr;
    MX_COMPTIME_VALUE_STRUCT_DECL, // struct MyStruct {}

    // Statements
    MX_COMPTIME_VALUE_EXPR_STMT,     // expr;
    MX_COMPTIME_VALUE_BREAK_STMT,    // break;
    MX_COMPTIME_VALUE_CONTINUE_STMT, // continue;
    MX_COMPTIME_VALUE_RETURN_STMT,   // return expr;
    MX_COMPTIME_VALUE_IF_STMT,       // if cond { }
    MX_COMPTIME_VALUE_LOOP_STMT,     // loop {}
    MX_COMPTIME_VALUE_FOR_STMT,      // for i in expr {}
    MX_COMPTIME_VALUE_ASSIGN_STMT,   // a = b;

    // Expressions
    MX_COMPTIME_VALUE_UNARY_EXPR,  // -1, not true
    MX_COMPTIME_VALUE_BINARY_EXPR, // a + b, a * b
    MX_COMPTIME_VALUE_RANGE_EXPR,  // a..b
    // Primary Expressions
    MX_COMPTIME_VALUE_INT_LITERAL,    // 42
    MX_COMPTIME_VALUE_FLOAT_LITERAL,  // 42.0
    MX_COMPTIME_VALUE_STRING_LITERAL, // "Hello world"
    MX_COMPTIME_VALUE_BOOL_LITERAL,   // true, false
    MX_COMPTIME_VALUE_LIST_LITERAL,   // [1, 2, 3]
    MX_COMPTIME_VALUE_MAP_LITERAL,    // map [ k: v ]
    MX_COMPTIME_VALUE_IDENTIFIER,     // my_ident
    MX_COMPTIME_VALUE_ELLIPSIS_EXPR,  // ...
    MX_COMPTIME_VALUE_CALL_EXPR,      // my_fn()
    MX_COMPTIME_VALUE_MEMBER_EXPR,    // a.b
    MX_COMPTIME_VALUE_STRUCT_EXPR,    // new MyStruct {}
    MX_COMPTIME_VALUE_GROUP_EXPR,     // (expr)
    MX_COMPTIME_VALUE_BLOCK,          // { }

    // Comptime Evaluation Result Values
    MX_COMPTIME_VALUE_TYPE,
    MX_COMPTIME_VALUE_STRUCT,
    MX_COMPTIME_VALUE_FN,

    // C Types
    MX_COMPTIME_VALUE_C_TYPE,
    MX_COMPTIME_VALUE_C_VALUE,

    MX_COMPTIME_VALUE_ENUM_COUNT
} MXComptimeValueKind;

typedef struct MXComptimeValue MXComptimeValue;
typedef ArrayList(MXComptimeValue) MXComptimeValueList;

typedef struct {
    MXComptimeValueList children;
} MXComptimeValueModule;

typedef struct {
    char *fn_name;
    MXComptimeValueList params;
    MXComptimeValue *expr;
} MXComptimeValueFnDecl;

typedef struct {
    char *var_name;
    MXComptimeValue *expr;
} MXComptimeValueVarDecl;

// Add similar struct definitions for each enum value as required:
typedef struct {
    char *struct_name;
    MXComptimeValueList members;
} MXComptimeValueStructDecl;

typedef struct {
    MXComptimeValue *expr;
} MXComptimeValueExprStmt;

typedef struct {
    // No additional fields
    uint8_t _;
} MXComptimeValueBreakStmt;

typedef struct {
    // No additional fields
    uint8_t _;
} MXComptimeValueContinueStmt;

typedef struct {
    MXComptimeValue *expr;
} MXComptimeValueReturnStmt;

typedef struct {
    MXComptimeValue *cond;
    MXComptimeValueList body;
} MXComptimeValueIfStmt;

typedef struct {
    MXComptimeValueList body;
} MXComptimeValueLoopStmt;

typedef struct {
    char *iter_var;
    MXComptimeValue *expr;
    MXComptimeValueList body;
} MXComptimeValueForStmt;

typedef struct {
    MXComptimeValue *lhs;
    MXComptimeValue *rhs;
} MXComptimeValueAssignStmt;

typedef struct {
    uint64_t int_value;
} MXComptimeValueIntLiteral;

typedef struct {
    double float_value;
} MXComptimeValueFloatLiteral;

typedef struct {
    char *string_value;
} MXComptimeValueStringLiteral;

typedef struct {
    bool bool_value;
} MXComptimeValueBoolLiteral;

typedef struct {
    MXComptimeValueList items;
} MXComptimeValueListLiteral;

typedef struct {
    MXComptimeValueList keys;
    MXComptimeValueList values;
} MXComptimeValueMapLiteral;

typedef struct {
    char *identifier_name;
} MXComptimeValueIdentifier;

typedef struct {
    MXComptimeValue *expr;
    MXComptimeValueList args;
} MXComptimeValueCallExpr;

typedef struct {
    MXComptimeValue *base;
    char *member_name;
} MXComptimeValueMemberExpr;

typedef struct {
    CType type;
} MXComptimeValueCType;

typedef struct {
    CType type;
    union {
        bool bool_value;
        int int_value;
        int8_t int8_value;
        int16_t int16_value;
        int32_t int32_value;
        int64_t int64_value;
        unsigned int uint_value;
        uint8_t uint8_value;
        uint16_t uint16_value;
        uint32_t uint32_value;
        uint64_t uint64_value;
    };
} MXComptimeValueCValue;

struct MXComptimeValue {
    MXComptimeValueKind kind;
    union {
        MXComptimeValueModule module;
        MXComptimeValueFnDecl fn_decl;
        MXComptimeValueVarDecl var_decl;
        MXComptimeValueStructDecl struct_decl;
        MXComptimeValueExprStmt expr_stmt;
        MXComptimeValueBreakStmt break_stmt;
        MXComptimeValueContinueStmt continue_stmt;
        MXComptimeValueReturnStmt return_stmt;
        MXComptimeValueIfStmt if_stmt;
        MXComptimeValueLoopStmt loop_stmt;
        MXComptimeValueForStmt for_stmt;
        MXComptimeValueAssignStmt assign_stmt;
        MXComptimeValueIntLiteral int_literal;
        MXComptimeValueFloatLiteral float_literal;
        MXComptimeValueStringLiteral string_literal;
        MXComptimeValueBoolLiteral bool_literal;
        MXComptimeValueListLiteral list_literal;
        MXComptimeValueMapLiteral map_literal;
        MXComptimeValueIdentifier identifier;
        MXComptimeValueCallExpr call_expr;
        MXComptimeValueMemberExpr member_expr;
        MXComptimeValueCType c_type;
        MXComptimeValueCValue c_value;
    };
};

typedef struct {
    const char *name;
    MXComptimeValue value;
    TSNode node;
} MXComptimeBinding;

typedef struct MXComptimeEnv {
    Arena *a;
    struct MXComptimeEnv *parent;
    HashMap *members;
    TSNode node;
} MXComptimeEnv;

typedef ArrayList(MXComptimeEnv *) MXComptimeEnvRefList;

MXComptimeEnv *mx_comptime_env_new(Arena *a, MXComptimeEnv *parent,
                                   TSNode node);

bool mx_comptime_env_declare(MXComptimeEnv *env, const char *name, TSNode node);

bool mx_comptime_env_set(MXComptimeEnv *env, const char *name,
                         MXComptimeValue value);

/// Check if the given name is declared in the current env. If recurse is true
/// then we will also check the parent envs.
bool mx_comptime_env_contains(MXComptimeEnv *env, const char *name,
                              bool recurse);

void mx_comptime_env_dump(MXComptimeEnv *env, int indent, bool show_nodes,
                          const char *src);

#endif // COMPTIME_VALUE_H
