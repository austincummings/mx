#ifndef STATIC_VALUE_H
#define STATIC_VALUE_H

#include "array_list.h"
typedef enum {
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
} MXStaticValueKind;

typedef struct MXStaticValue MXStaticValue;
typedef ArrayList(MXStaticValue) MXStaticValueList;

typedef struct {
    MXStaticValueList children;
} MXStaticValueModule;

struct MXStaticValue {
    MXStaticValueKind kind;
    union {
        MXStaticValueModule module;
    };
};

typedef struct {
} MXStaticEnv;

// Conversion Rules:
// Declarations are translated as is
// Statements are translated as is
// Expressions:
// Static expressions are evaluated

#endif // STATIC_VALUE_H
