#ifndef STATIC_VALUE_H
#define STATIC_VALUE_H

#include "array_list.h"
#include "map.h"

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
    MX_STATIC_VALUE_EMPTY,
    // Code Static Values
    // Declarations
    MX_STATIC_VALUE_MODULE,      // <list of stmts and decls>
    MX_STATIC_VALUE_FN_DECL,     // fn f(): static_expr {}
    MX_STATIC_VALUE_VAR_DECL,    // var my_var = expr;
    MX_STATIC_VALUE_STRUCT_DECL, // struct MyStruct {}

    // Statements
    MX_STATIC_VALUE_EXPR_STMT,     // expr;
    MX_STATIC_VALUE_BREAK_STMT,    // break;
    MX_STATIC_VALUE_CONTINUE_STMT, // continue;
    MX_STATIC_VALUE_RETURN_STMT,   // return expr;
    MX_STATIC_VALUE_IF_STMT,       // if cond { }
    MX_STATIC_VALUE_LOOP_STMT,     // loop {}
    MX_STATIC_VALUE_FOR_STMT,      // for i in expr {}
    MX_STATIC_VALUE_ASSIGN_STMT,   // a = b;

    // Expressions
    MX_STATIC_VALUE_UNARY_EXPR,  // -1, not true
    MX_STATIC_VALUE_BINARY_EXPR, // a + b, a * b
    MX_STATIC_VALUE_RANGE_EXPR,  // a..b
    // Primary Expressions
    MX_STATIC_VALUE_INT_LITERAL,    // 42
    MX_STATIC_VALUE_FLOAT_LITERAL,  // 42.0
    MX_STATIC_VALUE_STRING_LITERAL, // "Hello world"
    MX_STATIC_VALUE_BOOL_LITERAL,   // true, false
    MX_STATIC_VALUE_LIST_LITERAL,   // [1, 2, 3]
    MX_STATIC_VALUE_MAP_LITERAL,    // map [ k: v ]
    MX_STATIC_VALUE_IDENTIFIER,     // my_ident
    MX_STATIC_VALUE_ELLIPSIS_EXPR,  // ...
    MX_STATIC_VALUE_CALL_EXPR,      // my_fn()
    MX_STATIC_VALUE_MEMBER_EXPR,    // a.b
    MX_STATIC_VALUE_STRUCT_EXPR,    // new MyStruct {}
    MX_STATIC_VALUE_GROUP_EXPR,     // (expr)
    MX_STATIC_VALUE_BLOCK,          // { }

    // Static Evaluation Result Values
    MX_STATIC_VALUE_TYPE,
    MX_STATIC_VALUE_STRUCT,
    MX_STATIC_VALUE_FN,

    // C Types
    MX_STATIC_VALUE_C_TYPE,
    MX_STATIC_VALUE_C_VALUE
} MXStaticValueKind;

typedef struct MXStaticValue MXStaticValue;
typedef ArrayList(MXStaticValue) MXStaticValueList;

typedef struct {
    MXStaticValueList children;
} MXStaticValueModule;

typedef struct {
    char *fn_name;
    MXStaticValueList params;
    MXStaticValue *expr;
} MXStaticValueFnDecl;

typedef struct {
    char *var_name;
    MXStaticValue *expr;
} MXStaticValueVarDecl;

// Add similar struct definitions for each enum value as required:
typedef struct {
    char *struct_name;
    MXStaticValueList members;
} MXStaticValueStructDecl;

typedef struct {
    MXStaticValue *expr;
} MXStaticValueExprStmt;

typedef struct {
    // No additional fields
    uint8_t _;
} MXStaticValueBreakStmt;

typedef struct {
    // No additional fields
    uint8_t _;
} MXStaticValueContinueStmt;

typedef struct {
    MXStaticValue *expr;
} MXStaticValueReturnStmt;

typedef struct {
    MXStaticValue *cond;
    MXStaticValueList body;
} MXStaticValueIfStmt;

typedef struct {
    MXStaticValueList body;
} MXStaticValueLoopStmt;

typedef struct {
    char *iter_var;
    MXStaticValue *expr;
    MXStaticValueList body;
} MXStaticValueForStmt;

typedef struct {
    MXStaticValue *lhs;
    MXStaticValue *rhs;
} MXStaticValueAssignStmt;

typedef struct {
    uint64_t int_value;
} MXStaticValueIntLiteral;

typedef struct {
    double float_value;
} MXStaticValueFloatLiteral;

typedef struct {
    char *string_value;
} MXStaticValueStringLiteral;

typedef struct {
    bool bool_value;
} MXStaticValueBoolLiteral;

typedef struct {
    MXStaticValueList items;
} MXStaticValueListLiteral;

typedef struct {
    MXStaticValueList keys;
    MXStaticValueList values;
} MXStaticValueMapLiteral;

typedef struct {
    char *identifier_name;
} MXStaticValueIdentifier;

typedef struct {
    MXStaticValue *expr;
    MXStaticValueList args;
} MXStaticValueCallExpr;

typedef struct {
    MXStaticValue *base;
    char *member_name;
} MXStaticValueMemberExpr;

typedef struct {
    CType type;
} MXStaticValueCType;

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
} MXStaticValueCValue;

struct MXStaticValue {
    MXStaticValueKind kind;
    union {
        MXStaticValueModule module;
        MXStaticValueFnDecl fn_decl;
        MXStaticValueVarDecl var_decl;
        MXStaticValueStructDecl struct_decl;
        MXStaticValueExprStmt expr_stmt;
        MXStaticValueBreakStmt break_stmt;
        MXStaticValueContinueStmt continue_stmt;
        MXStaticValueReturnStmt return_stmt;
        MXStaticValueIfStmt if_stmt;
        MXStaticValueLoopStmt loop_stmt;
        MXStaticValueForStmt for_stmt;
        MXStaticValueAssignStmt assign_stmt;
        MXStaticValueIntLiteral int_literal;
        MXStaticValueFloatLiteral float_literal;
        MXStaticValueStringLiteral string_literal;
        MXStaticValueBoolLiteral bool_literal;
        MXStaticValueListLiteral list_literal;
        MXStaticValueMapLiteral map_literal;
        MXStaticValueIdentifier identifier;
        MXStaticValueCallExpr call_expr;
        MXStaticValueMemberExpr member_expr;
        MXStaticValueCType c_type;
        MXStaticValueCValue c_value;
    };
};

typedef struct StaticEnv {
    struct StaticEnv *parent;
    HashMap *members;
} StaticEnv;

StaticEnv *static_env_new(Arena *a, StaticEnv *parent);

#endif // STATIC_VALUE_H
