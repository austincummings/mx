#ifndef PARSER_H
#define PARSER_H

#include <tree_sitter/api.h>

#include "arena.h"
#include "array_list.h"

const TSLanguage *tree_sitter_mx(void);

typedef int32_t Index;

typedef struct {
    TSPoint start;
    TSPoint end;
} ParserError;

typedef ArrayList(ParserError) ParserErrorList;
typedef ArrayList(TSNode) NodeList;

typedef struct {
    Arena a;
    TSTreeCursor cursor;
    const char *src;

    NodeList nodes;

    ParserErrorList errors;
} MXParser;

MXParser mx_parser_new(const char *src);

void mx_parser_parse(MXParser *self);

void mx_parser_report_errors(MXParser *self);

Index mx_parser_append_node(MXParser *self, TSNode node);

#endif // PARSER_H
