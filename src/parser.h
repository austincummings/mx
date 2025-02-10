#include "ast.h"
#include "mem.h"

typedef struct {
    Arena arena;
} MXParser;

void mx_parser_init(MXParser *parser, const char *src);

Ast mx_parser_parse(MXParser *parser);
