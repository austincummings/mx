#ifndef SEMA_H
#define SEMA_H

#include "comptime_value.h"
#include "parser.h"
#include <tree_sitter/api.h>

typedef struct {
    Arena a;
    MXComptimeEnvRefList envs;
    const char *src;
    TSTree *tree;
} MXSema;

MXSema mx_sema_new(const char *src);

void mx_sema_analyze(MXSema *self);

#endif
