#ifndef SEMA_H
#define SEMA_H

#include "parser.h"
#include "comptime_value.h"
#include <tree_sitter/api.h>

typedef struct {
    Arena a;
    const char *src;

    TSTree *tree;
} MXSema;

MXSema mx_semantic_analyzer_new(const char *src);

void mx_semantic_analyzer_analyze(MXSema *self);

#endif
