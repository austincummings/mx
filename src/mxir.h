#ifndef _MX_MXIR_H
#define _MX_MXIR_H

#include <stdint.h>

#include "array_list.h"
#include "ast.h"
#include "diag.h"

typedef uint32_t MxirNodeRef;

typedef struct {
    MxirNodeRef node;
    bool is_present;
} OptionalMxirNodeRef;

typedef ArrayList(MxirNodeRef) MxirNodeRefList;

typedef enum {
    MXIR_FN,
    MXIR_VAR_DECL,

    MXIR_CALL,
    MXIR_VAR_ACCESS,
    MXIR_MEMBER_ACCESS,
    MXIR_ASSIGN,
    MXIR_RETURN,
    MXIR_CONTINUE,
    MXIR_BREAK,
    MXIR_IF,
    MXIR_LOOP,
    MXIR_BLOCK,

    // Values
    MXIR_COMPTIME_STRING,
    MXIR_COMPTIME_INT,
    MXIR_COMPTIME_FLOAT,
    MXIR_COMPTIME_BOOL,
    MXIR_COMPTIME_C_VALUE,
    MXIR_COMPTIME_C_TYPE,
} MxirNodeKind;

typedef struct {
    MxirNodeRef self_ref;
    MxirNodeKind kind;
    AstNodeRef ast_node;

    union {
        struct {
            const char *name;
            MxirNodeRefList params;
            MxirNodeRef body;
        } fn;
        struct {
            const char *name;
            MxirNodeRef value_expr;
        } var_decl;
        struct {
            MxirNodeRef fn;
            MxirNodeRefList args;
        } call;
        struct {
            const char *name;
        } var_access;
        struct {
            MxirNodeRef var;
            const char *member;
        } member_access;
        struct {
            MxirNodeRef var;
            MxirNodeRef value;
        } assign;
        struct {
            MxirNodeRef value;
        } return_;
        struct {
            MxirNodeRef cond;
            MxirNodeRef then;
            OptionalMxirNodeRef else_;
        } if_;
        struct {
            MxirNodeRef body;
        } loop;
        struct {
            MxirNodeRefList nodes;
        } block;

        // Values
        struct {
            const char *value;
        } comptime_string;
        struct {
            int64_t value;
        } comptime_int;
        struct {
            double value;
        } comptime_float;
        struct {
            bool value;
        } comptime_bool;
        struct {
            const char *value;
        } comptime_c_value;
        struct {
            const char *value;
        } comptime_c_type;
    };
} MxirNode;

typedef ArrayList(MxirNode) MxirNodeList;

typedef struct {
    const char *src;    // Owned by the permanent arena
    MxirNodeList nodes; // Owned by the permanent arena
    MXDiagnosticList diagnostics;
} Mxir;

#endif // _MX_MXIR_H
