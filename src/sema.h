#ifndef SEMA_H
#define SEMA_H

#include "arena.h"
#include "comptime_value.h"
#include <tree_sitter/api.h>

typedef struct {
    Arena a;
    const char *src;
    TSTree *tree;
    MXComptimeEnvRefList envs;
} MXSema;

MXSema mx_sema_new(const char *src);

void mx_sema_analyze(MXSema *self);

#endif
