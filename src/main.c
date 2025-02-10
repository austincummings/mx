#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "io.h"
#include "mem.h"
#include "parser.h"
#include "server.h"

Arena permanent_arena = {0};

Ast parse(const char *src) {
    MXParser parser = {0};
    mx_parser_init(&permanent_arena, &parser, src);
    Ast ast = mx_parser_parse(&parser);
    return ast;
}

int analyze() { return 0; }

int codegen() { return 0; }

const char *emit() { return "int main() { return 0; }"; }

int compile() {
    const char *src = read_from_stdin(&permanent_arena);
    Ast ast = parse(src);
    analyze();
    codegen();
    const char *out = emit();
    printf("%s\n", out);
    return 0;
}

int server() {
    MXLangServer server = {0};
    mx_lang_server_init(&permanent_arena, &server);
    mx_lang_server_listen(&server);
    return 0;
}

void usage() { printf("Usage: mxc <server|compile>\n"); }

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage();
        return 1;
    }

    arena_init(&permanent_arena, ARENA_DEFAULT_RESERVE_SIZE);

    if (strcmp(argv[1], "server") == 0) {
        return server();
    } else if (strcmp(argv[1], "compile") == 0) {
        return compile();
    } else {
        usage();
        return 1;
    }

    return 0;
}
