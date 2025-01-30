#ifndef COMPTIME_VALUE_H
#define COMPTIME_VALUE_H

#include "array_list.h"
#include "map.h"
#include "ts_ext.h"
#include <tree_sitter/api.h>

typedef struct {
    const char *name;
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

MXComptimeBinding *mx_comptime_env_get(MXComptimeEnv *env, const char *name,
                                       bool recurse);

/// Check if the given name is declared in the current env. If recurse is true
/// then we will also check the parent envs.
bool mx_comptime_env_contains(MXComptimeEnv *env, const char *name,
                              bool recurse);

void mx_comptime_env_dump(MXComptimeEnv *env, int indent, bool show_nodes,
                          const char *src);

#endif // COMPTIME_VALUE_H
