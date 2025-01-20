#include "sema.h"
#include "arena.h"
#include "array_list.h"
#include "debug.h"
#include "ts_ext.h"

static void walk_tree(MXSemanticAnalyzer *sema) {
    Arena scratch = {0};
    arena_init(&scratch, ARENA_DEFAULT_RESERVE_SIZE);

    do {
        TSNode node = ts_tree_cursor_current_node(&sema->cursor);
        const char *node_type = ts_node_type(node);

        if (ts_node_is_error(node)) {
            // TSPoint start = ts_node_start_point(node);
            // TSPoint end = ts_node_end_point(node);
            // ParserError error = (ParserError){.start = start, .end = end};
            // arraylist_add(&parser->a, &parser->errors, error);
            continue;
        }

        if (!ts_node_is_named(node)) {
            continue;
        }
        debug("node _type: %s\n", node_type);

        if (ts_tree_cursor_goto_first_child(&sema->cursor)) {
            walk_tree(sema);
            ts_tree_cursor_goto_parent(&sema->cursor);
        }
    } while (ts_tree_cursor_goto_next_sibling(&sema->cursor));

    arena_release(&scratch);
}

MXSemanticAnalyzer mx_semantic_analyzer_new(const char *src) {
    MXSemanticAnalyzer self = {0};

    self.a = (Arena){0};
    arena_init(&self.a, ARENA_DEFAULT_RESERVE_SIZE);

    self.src = src;

    return self;
}

void mx_semantic_analyzer_analyze(MXSemanticAnalyzer *self) {
    assert(self != NULL);

    TSNode root_node = ts_tree_root_node(self->tree);
    assert(!ts_node_is_null(root_node));
    self->cursor = ts_tree_cursor_new(root_node);
    walk_tree(self);
}
