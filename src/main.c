#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "io.h"
#include "mem.h"
#include "parser.h"
#include "sema.h"
#include "server.h"

Arena permanent_arena = {0};

int codegen() { return 0; }

const char *emit() { return "int main() { return 0; }"; }

int compile() {
    const char *src = read_from_stdin(&permanent_arena);
    Ast *ast = parse(&permanent_arena, src);
    assert(ast != NULL);
    analyze(&permanent_arena, ast);
    codegen();
    const char *out = emit();
    printf("%s\n", out);
    return 0;
}

void usage() {
    printf("Usage: mxc [COMMAND]\n\nCommands:\n    compile\n    server\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage();
        return 1;
    }

    arena_init(&permanent_arena, ARENA_DEFAULT_RESERVE_SIZE);

    if (strcmp(argv[1], "server") == 0) {
        server(&permanent_arena);
        return 0;
    } else if (strcmp(argv[1], "compile") == 0) {
        return compile();
    } else {
        usage();
        return 1;
    }

    return 0;
}
