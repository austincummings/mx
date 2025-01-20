#include <assert.h>
#include <stdio.h>

#include "arena.h"
#include "debug.h"
#include "parser.h"
#include "sema.h"

void usage(const char **argv) {
    fprintf(stderr, "Usage: %s <path>\n", argv[0]);
}

const char *read_file(Arena *a, const char *path) {
    assert(path != NULL);
    assert(a != NULL);

    FILE *f = fopen(path, "rb");
    assert(f != NULL);

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    const char *buffer = arena_alloc(a, size);
    fread((void *)buffer, 1, size, f);
    return buffer;
}

int main(int argc, const char **argv) {
    const char *path = NULL;
    if (argc > 1) {
        path = argv[1];
    } else {
        usage(argv);
        return 1;
    }
    assert(path != NULL);

    Arena main_arena = {0};
    arena_init(&main_arena, ARENA_DEFAULT_RESERVE_SIZE);

    const char *src = read_file(&main_arena, argv[1]);

    MXParser parser = mx_parser_new(src);
    mx_parser_parse(&parser);

    if (parser.errors.size > 0) {
        mx_parser_report_errors(&parser);
        return 1;
    }

    MXSemanticAnalyzer sema = mx_semantic_analyzer_new(src);
    sema.tree = parser.tree;
    mx_semantic_analyzer_analyze(&sema);

    const char *c_code = "";
    printf("%s\n", c_code);

    return 0;
}
