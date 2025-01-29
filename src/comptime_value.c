#include "comptime_value.h"
#include "debug.h"
#include "map.h"
#include "ts_ext.h"
#include <tree_sitter/api.h>

const char *MXComptimeValueKindString[MX_COMPTIME_VALUE_ENUM_COUNT] = {
    "MX_COMPTIME_VALUE_UNDEFINED",
    "MX_COMPTIME_VALUE_FN_DECL",
    "MX_COMPTIME_VALUE_STRUCT_DECL",
    "MX_COMPTIME_VALUE_INT_LITERAL",
    "MX_COMPTIME_VALUE_FLOAT_LITERAL",
    "MX_COMPTIME_VALUE_STRING_LITERAL",
    "MX_COMPTIME_VALUE_BOOL_LITERAL",
    "MX_COMPTIME_VALUE_IDENTIFIER",
    "MX_COMPTIME_VALUE_FN",
    "MX_COMPTIME_VALUE_C_TYPE",
    "MX_COMPTIME_VALUE_C_VALUE",
};

MXComptimeEnv *mx_comptime_env_new(Arena *a, MXComptimeEnv *parent,
                                   TSNode node) {
    MXComptimeEnv *self = arena_alloc_struct(a, MXComptimeEnv);
    self->a = a;
    self->parent = parent;
    self->members = hashmap_init(a);
    self->node = node;
    return self;
}

bool mx_comptime_env_declare(MXComptimeEnv *env, const char *name,
                             TSNode node) {
    assert(env != NULL);
    assert(name != NULL);
    assert(strlen(name) > 0);
    assert(!ts_node_is_null(node));

    // If the name is already declared in the current env then return false
    // otherwise add the name to the current env and return true.
    if (mx_comptime_env_contains(env, name, false)) {
        return false;
    }

    MXComptimeBinding *binding = arena_alloc_struct(env->a, MXComptimeBinding);
    binding->name = name;
    binding->value = (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_UNDEFINED,
    };
    binding->node = node;
    hashmap_set(env->a, env->members, name, binding);

    return true;
}

bool mx_comptime_env_set(MXComptimeEnv *env, const char *name,
                         MXComptimeValue value) {
    assert(env != NULL);
    assert(name != NULL);
    assert(strlen(name) > 0);

    // First find which environment the name is declared in.
    MXComptimeEnv *decl_env = env;
    while (decl_env != NULL) {
        if (hashmap_get(decl_env->members, name) != NULL) {
            break;
        }
        decl_env = decl_env->parent;
    }

    // If the name is not declared in any environment then return false.
    if (decl_env == NULL) {
        return false;
    }

    // If the name is declared in the current env then update the value and
    // return true.
    MXComptimeBinding *binding = hashmap_get(decl_env->members, name);
    if (binding != NULL) {
        binding->value = value;
        return true;
    }

    // This is unreachable so we should abort.
    debug("unreachable\n");
    abort();
}

MXComptimeBinding *mx_comptime_env_get(MXComptimeEnv *env, const char *name,
                                       bool recurse) {
    assert(env != NULL);
    assert(name != NULL);

    MXComptimeEnv *decl_env = env;
    while (decl_env != NULL) {
        MXComptimeBinding *binding = hashmap_get(decl_env->members, name);
        if (binding != NULL) {
            return binding;
        }

        if (!recurse) {
            break;
        }

        decl_env = decl_env->parent;
    }

    return NULL;
}

bool mx_comptime_env_contains(MXComptimeEnv *env, const char *name,
                              bool recurse) {
    assert(env != NULL);
    assert(name != NULL);

    if (hashmap_get(env->members, name) != NULL) {
        return true;
    }

    if (recurse && env->parent != NULL) {
        return mx_comptime_env_contains(env->parent, name, true);
    }

    return false;
}

void mx_comptime_env_dump(MXComptimeEnv *env, int indent, bool show_nodes,
                          const char *src) {
    assert(env != NULL);

    const char *node_line = ts_node_line_text(env->a, env->node, src);
    debug("%*senv(%p): %s\n", indent, "", env, node_line);

    if (env->members->size == 0) {
        debug("%*s<empty>\n", indent + 2, "");
    }
    HashMapIterator it = hashmap_iterator(env->members);
    while (hashmap_iterator_has_next(&it)) {
        const char *key = NULL;
        MXComptimeBinding *binding = NULL;
        hashmap_iterator_next(&it, &key, (void **)&binding);
        const char *kind = MXComptimeValueKindString[binding->value.kind];
        if (show_nodes && !ts_node_is_null(binding->node)) {
            const char *node_type = ts_node_type(binding->node);
            debug("%*s%s = %s %s(%p)\n", indent + 2, "", key, kind, node_type,
                  binding->node.id);

        } else {
            debug("%*s%s = %s\n", indent + 2, "", key, kind);
        }
    }

    if (env->parent != NULL) {
        mx_comptime_env_dump(env->parent, indent + 4, show_nodes, src);
    }
}

MXComptimeValue mx_comptime_undefined() {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_UNDEFINED,
    };
}

MXComptimeValue mx_comptime_fn_decl(const char *name,
                                    TSNodeList *comptime_params,
                                    TSNodeList *params, TSNode return_type,
                                    TSNode body) {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_FN_DECL,
        .fn_decl =
            (MXComptimeValueFnDecl){
                .name = name,
                .comptime_params = comptime_params,
                .params = params,
                .return_type = return_type,
                .body = body,
            },
    };
}

MXComptimeValue mx_comptime_struct_decl(const char *name) {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_STRUCT_DECL,
        .struct_decl = (MXComptimeValueStructDecl){.name = name},
    };
}

MXComptimeValue mx_comptime_fn(const char *name) {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_FN,
        .fn = (MXComptimeValueFn){.name = name},
    };
}

MXComptimeValue mx_comptime_struct(const char *name) {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_STRUCT,
        .struct_ = (MXComptimeValueStruct){.name = name},
    };
}

MXComptimeValue mx_comptime_int_literal(uint64_t value) {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_INT_LITERAL,
        .int_literal = (MXComptimeValueIntLiteral){.int_value = value},
    };
}

MXComptimeValue mx_comptime_float_literal(double value) {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_FLOAT_LITERAL,
        .float_literal = (MXComptimeValueFloatLiteral){.float_value = value},
    };
}

MXComptimeValue mx_comptime_string_literal(const char *value) {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_STRING_LITERAL,
        .string_literal = (MXComptimeValueStringLiteral){.string_value = value},
    };
}

MXComptimeValue mx_comptime_bool_literal(bool value) {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_BOOL_LITERAL,
        .bool_literal = (MXComptimeValueBoolLiteral){.bool_value = value},
    };
}

MXComptimeValue mx_comptime_c_type(CType type) {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_C_TYPE,
        .c_type = (MXComptimeValueCType){.type = type},
    };
}

MXComptimeValue mx_comptime_c_value(CType type, CValue value) {
    return (MXComptimeValue){
        .kind = MX_COMPTIME_VALUE_C_VALUE,
        .c_value = (MXComptimeValueCValue){.type = type, .value = value},
    };
}
