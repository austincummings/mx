#include <tree_sitter/api.h>

#include "arena.h"
#include "parser.h"

static void walk_tree(MXParser *parser) {
    Arena scratch = {0};
    arena_init(&scratch, ARENA_DEFAULT_RESERVE_SIZE);

    do {
        TSNode node = ts_tree_cursor_current_node(&parser->cursor);

        if (ts_node_is_error(node)) {
            TSPoint start = ts_node_start_point(node);
            TSPoint end = ts_node_end_point(node);
            ParserError error = (ParserError){.start = start, .end = end};
            arraylist_add(&parser->a, &parser->errors, error);
            continue;
        }

        if (!ts_node_is_named(node)) {
            continue;
        }

        if (ts_tree_cursor_goto_first_child(&parser->cursor)) {
            walk_tree(parser);
            ts_tree_cursor_goto_parent(&parser->cursor);
        }
    } while (ts_tree_cursor_goto_next_sibling(&parser->cursor));

    arena_release(&scratch);
}

MXParser mx_parser_new(const char *src) {
    MXParser self = {0};

    self.a = (Arena){0};
    arena_init(&self.a, ARENA_DEFAULT_RESERVE_SIZE);

    self.src = src;

    self.errors = (ParserErrorList){0};
    arraylist_init(&self.a, &self.errors, 1024);

    return self;
}

void mx_parser_parse(MXParser *self) {
    assert(self != NULL);
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_mx());

    TSTree *tree =
        ts_parser_parse_string(parser, NULL, self->src, strlen(self->src));
    assert(tree != NULL);

    self->tree = tree;

    TSNode root_node = ts_tree_root_node(tree);
    assert(!ts_node_is_null(root_node));

    self->cursor = ts_tree_cursor_new(root_node);

    walk_tree(self);
}

void mx_parser_report_errors(MXParser *self) {
    for (size_t i = 0; i < self->errors.size; i++) {
        ParserError *error = arraylist_get(&self->errors, i);
        fprintf(stderr, "Syntax error at: %d:%d - %d:%d\n",
                error->start.row + 1, error->start.column, error->end.row + 1,
                error->end.column);
    }
}
