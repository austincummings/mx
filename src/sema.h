#ifndef SEMA_H
#define SEMA_H

#include "parser.h"
#include "static_value.h"
#include <tree_sitter/api.h>

typedef struct {
    Arena a;
    const char *src;

    TSTree *tree;
} MXSemanticAnalyzer;

MXSemanticAnalyzer mx_semantic_analyzer_new(const char *src);

void mx_semantic_analyzer_analyze(MXSemanticAnalyzer *self);

#endif
