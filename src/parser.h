#ifndef _MX_PARSER_H
#define _MX_PARSER_H

#include "ast.h"
#include "diag.h"
#include "mem.h"
#include <tree_sitter/api.h>

const TSLanguage *tree_sitter_mx(void);

typedef struct {
    Arena *permanent_arena;
    Arena arena;

    const char *src;

    TSTreeCursor cursor;
    TSTree *tree;

    MXDiagnosticList diagnostics;
} MXParser;

void mx_parser_init(Arena *permanent_arena, MXParser *parser, const char *src);

Ast mx_parser_parse(MXParser *parser);

#endif // _MX_PARSER_H
