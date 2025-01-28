#include "comptime_value.h"

MXComptimeEnv *mx_comptime_env_new(Arena *a, MXComptimeEnv *parent) {
    MXComptimeEnv *self = arena_alloc_struct(a, MXComptimeEnv);
    self->parent = parent;
    self->members = hashmap_init(a);
    return self;
}

void mx_comptime_env_set(MXComptimeEnv *env, const char *name,
                         MXComptimeValue value) {
    assert(env != NULL);
    assert(name != NULL);
    assert(strlen(name) > 0);
    // Search in the current and parent envs to see if there is already a name
    // bound. If we found a binding then we can set the value, otherwise we need
    // to create a binding for it and assign the value.
}

bool mx_comptime_env_contains(MXComptimeEnv *env, const char *name) {}
