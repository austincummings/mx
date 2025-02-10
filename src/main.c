#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "mem.h"

int parse(const char *src) { return 0; }

int analyze() { return 0; }

int codegen() { return 0; }

const char *emit() { return "int main() { return 0; }"; }

int compile() {
    parse("");
    analyze();
    codegen();
    const char *src = emit();
    printf("%s\n", src);
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
    setup_signal_handler();

    if (argc != 2) {
        usage();
        return 1;
    }

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
