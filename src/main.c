#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "io.h"
#include "mem.h"
#include "parser.h"

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
    Arena server_arena = {0};
    arena_init(&server_arena, ARENA_DEFAULT_RESERVE_SIZE);

    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (strstr(buffer, "Content-Length:") != NULL) {
            int content_length = 0;
            sscanf(buffer, "Content-Length: %d", &content_length);
            fgets(buffer, sizeof(buffer), stdin); // Read empty line

            if (content_length > 0 && content_length < sizeof(buffer)) {
                fread(buffer, 1, content_length, stdin);
                buffer[content_length] = '\0'; // Ensure null termination

                printf("Content-Length: "
                       "%d\r\n\r\n{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                       "\"Hello from the other side\"}\n",
                       (int)strlen("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                                   "\"Hello from the other side\"}"));
                fflush(stdout);
            }
        }
    }

    arena_release(&server_arena);
    return 0;
}

void usage() { printf("Usage: mxc <lsp|compile>\n"); }

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage();
        return 1;
    }

    arena_init(&permanent_arena, ARENA_DEFAULT_RESERVE_SIZE);

    if (strcmp(argv[1], "lsp") == 0) {
        return server();
    } else if (strcmp(argv[1], "compile") == 0) {
        return compile();
    } else {
        usage();
        return 1;
    }

    return 0;
}
