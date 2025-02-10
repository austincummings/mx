#include <stdio.h>
#include <string.h>
#include <tree_sitter/api.h>

#include "ast.h"
#include "debug.h"
#include "mem.h"
#include "parser.h"

static void walk_tree(MXParser *parser) {
    Arena scratch = {0};
    arena_init(&scratch, ARENA_DEFAULT_RESERVE_SIZE);

    do {
        TSNode node = ts_tree_cursor_current_node(&parser->cursor);

        // printf("%s(%b): ", ts_node_type(node), ts_node_is_named(node));
        // printf("`%.*s`\n",
        //        (int)ts_node_end_byte(node) - ts_node_start_byte(node),
        //        parser->src + ts_node_start_byte(node));
        if (!ts_node_is_named(node)) {
            continue;
        }

        MXRange range = {
            .start =
                {
                    .row = ts_node_start_point(node).row,
                    .col = ts_node_start_point(node).column,
                },
            .end =
                {
                    .row = ts_node_end_point(node).row,
                    .col = ts_node_end_point(node).column,
                },
        };

        const char *type_orig = ts_node_type(node);
        const char *type =
            arena_alloc(parser->permanent_arena, strlen(type_orig) + 1);
        memcpy((void *)type, type_orig, strlen(type_orig) + 1);
        const char *text =
            arena_alloc(parser->permanent_arena,
                        ts_node_end_byte(node) - ts_node_start_byte(node) + 1);
        memcpy((void *)text, parser->src + ts_node_start_byte(node),
               ts_node_end_byte(node) - ts_node_start_byte(node));

        // Store any syntax errors
        if (ts_node_is_error(node)) {
            MXDiagnostic diag = {
                .range = range,
                .kind = MX_DIAG_SYNTAX_ERROR,
            };
            arraylist_add(&parser->arena, &parser->diagnostics, diag);
        }

        if (ts_tree_cursor_goto_first_child(&parser->cursor)) {
            walk_tree(parser);
            ts_tree_cursor_goto_parent(&parser->cursor);
        }
    } while (ts_tree_cursor_goto_next_sibling(&parser->cursor));

    arena_release(&scratch);
}

void mx_parser_init(Arena *permanent_arena, MXParser *parser, const char *src) {
    parser->permanent_arena = permanent_arena;
    arena_init(&parser->arena, ARENA_DEFAULT_RESERVE_SIZE);
    parser->src = src;
}

Ast mx_parser_parse(MXParser *parser) {
    TSParser *ts_parser = ts_parser_new();
    ts_parser_set_language(ts_parser, tree_sitter_mx());

    TSTree *tree = ts_parser_parse_string(ts_parser, NULL, parser->src,
                                          strlen(parser->src));
    assert(tree != NULL);

    parser->tree = tree;

    TSNode root_node = ts_tree_root_node(tree);
    assert(!ts_node_is_null(root_node));

    parser->cursor = ts_tree_cursor_new(root_node);

    walk_tree(parser);

    Ast ast = {0};
    return ast;
}
