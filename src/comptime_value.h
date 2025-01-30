#ifndef COMPTIME_VALUE_H
#define COMPTIME_VALUE_H

#include "array_list.h"
#include "map.h"
#include "ts_ext.h"
#include <tree_sitter/api.h>

typedef enum {
    MX_COMPTIME_VALUE_UNDEFINED,

    MX_COMPTIME_VALUE_FN_DECL,     // fn f(): comptime_expr {}
    MX_COMPTIME_VALUE_STRUCT_DECL, // struct MyStruct {}

    MX_COMPTIME_VALUE_FN,      // a resolved function
    MX_COMPTIME_VALUE_STRUCT,  // a resolved struct
    MX_COMPTIME_VALUE_INT,     // 42
    MX_COMPTIME_VALUE_FLOAT,   // 42.0
    MX_COMPTIME_VALUE_STRING,  // "Hello world"
    MX_COMPTIME_VALUE_BOOL,    // true, false
    MX_COMPTIME_VALUE_C_TYPE,  // c_type["int"]
    MX_COMPTIME_VALUE_C_VALUE, // c_value[42]

    MX_COMPTIME_VALUE_ENUM_COUNT
} MXComptimeValueKind;

typedef struct MXComptimeValue MXComptimeValue;
typedef ArrayList(MXComptimeValue) MXComptimeValueList;

typedef struct {
    const char *name;
    MXComptimeValueList comptime_args;
} MXComptimeValueFn;

typedef ArrayList(MXComptimeValueFn) MXComptimeValueFnList;

typedef struct {
    const char *name;
    // TODO: Add resolved params and body
} MXComptimeValueStruct;

typedef struct {
    const char *name;
    TSNodeList *comptime_params;
    TSNodeList *params;
    TSNode return_type;
    TSNode body;

    MXComptimeValueFnList variants;
} MXComptimeValueFnDecl;

typedef struct {
    const char *name;
    // TODO: Expand
} MXComptimeValueStructDecl;

typedef struct {
    uint64_t value;
} MXComptimeValueInt;

typedef struct {
    double value;
} MXComptimeValueFloat;

typedef struct {
    const char *value;
} MXComptimeValueString;

typedef struct {
    bool value;
} MXComptimeValueBool;

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

typedef struct {
    CType type;
} MXComptimeValueCType;

typedef union {
    bool as_bool;
    int as_int;
    int8_t as_int8;
    int16_t as_int16;
    int32_t as_int32;
    int64_t as_int64;
    unsigned int as_unsigned_int;
    uint8_t as_uint8;
    uint16_t as_uint16;
    uint32_t as_uint32;
    uint64_t as_uint64;
} CValue;

typedef struct {
    CType type;
    CValue value;
} MXComptimeValueCValue;

struct MXComptimeValue {
    MXComptimeValueKind kind;
    union {
        MXComptimeValueFnDecl as_fn_decl;
        MXComptimeValueStructDecl as_struct_decl;
        MXComptimeValueFn as_fn;
        MXComptimeValueStruct as_struct;
        MXComptimeValueInt as_int;
        MXComptimeValueFloat as_float;
        MXComptimeValueString as_string;
        MXComptimeValueBool as_bool;
        MXComptimeValueCType as_c_type;
        MXComptimeValueCValue as_c_value;
    };
};

MXComptimeValue mx_comptime_undefined();

MXComptimeValue mx_comptime_fn_decl(const char *name,
                                    TSNodeList *comptime_params,
                                    TSNodeList *params, TSNode return_type,
                                    TSNode body);

MXComptimeValue mx_comptime_struct_decl(const char *name);

MXComptimeValue mx_comptime_fn(const char *name);

MXComptimeValue mx_comptime_struct(const char *name);

MXComptimeValue mx_comptime_int(uint64_t value);

MXComptimeValue mx_comptime_float(double value);

MXComptimeValue mx_comptime_string(const char *value);

MXComptimeValue mx_comptime_bool(bool value);

MXComptimeValue mx_comptime_c_type(CType type);

MXComptimeValue mx_comptime_c_value(CType type, CValue value);

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

MXComptimeBinding *mx_comptime_env_get(MXComptimeEnv *env, const char *name,
                                       bool recurse);

/// Check if the given name is declared in the current env. If recurse is true
/// then we will also check the parent envs.
bool mx_comptime_env_contains(MXComptimeEnv *env, const char *name,
                              bool recurse);

void mx_comptime_env_dump(MXComptimeEnv *env, int indent, bool show_nodes,
                          const char *src);

#endif // COMPTIME_VALUE_H
