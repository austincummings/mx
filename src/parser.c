#include "parser.h"
#include "ast.h"
#include "mem.h"

void mx_parser_init(MXParser *parser, const char *src) {
    arena_init(&parser->arena, ARENA_DEFAULT_RESERVE_SIZE);
}

Ast mx_parser_parse(MXParser *parser) {
    Ast ast = {0};
    return ast;
}
