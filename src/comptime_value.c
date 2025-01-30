#include "comptime_value.h"
#include "debug.h"
#include "map.h"
#include "ts_ext.h"
#include <tree_sitter/api.h>

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
    binding->node = node;
    hashmap_set(env->a, env->members, name, binding);

    return true;
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
        if (show_nodes && !ts_node_is_null(binding->node)) {
            const char *node_type = ts_node_type(binding->node);
            debug("%*s%s = %s(%p)\n", indent + 2, "", key, node_type,
                  binding->node.id);

        } else {
            debug("%*s%s\n", indent + 2, "", key);
        }
    }

    if (env->parent != NULL) {
        mx_comptime_env_dump(env->parent, indent + 4, show_nodes, src);
    }
}
