#include <stdio.h>
#include <string.h>
#include <tree_sitter/api.h>

#include "ast.h"
#include "mem.h"
#include "parser.h"

static AstNodeRef walk_tree(MXParser *parser, TSNode node) {
    Arena scratch = {0};
    arena_init(&scratch, ARENA_DEFAULT_RESERVE_SIZE);

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
        arraylist_add(parser->permanent_arena, &parser->diagnostics, diag);
    }

    AstNodeRef self_ref = parser->nodes.size;

    printf("%d: %s(%b): ", self_ref, ts_node_type(node),
           ts_node_is_named(node));
    printf("`%.*s`\n", (int)ts_node_end_byte(node) - ts_node_start_byte(node),
           parser->src + ts_node_start_byte(node));

    // Store the node
    AstNode ast_node = {.self_ref = self_ref,
                        .type = type,
                        .text = text,
                        .range = range,
                        .children = {0}};
    arraylist_init(parser->permanent_arena, &ast_node.children, 24);
    arraylist_add(parser->permanent_arena, &parser->nodes, ast_node);

    for (size_t i = 0; i < ts_node_named_child_count(node); i++) {
        TSNode child = ts_node_named_child(node, i);
        AstNodeRef child_ref = walk_tree(parser, child);
        arraylist_add(parser->permanent_arena, &ast_node.children, child_ref);
    }

    arena_release(&scratch);

    return self_ref;
}

void mx_parser_init(Arena *permanent_arena, MXParser *parser, const char *src) {
    parser->permanent_arena = permanent_arena;
    arena_init(&parser->arena, ARENA_DEFAULT_RESERVE_SIZE);
    parser->src = src;
    arraylist_init(&parser->arena, &parser->diagnostics, 24);
    arraylist_init(&parser->arena, &parser->nodes, 24);
}

void mx_parser_parse(MXParser *parser) {
    TSParser *ts_parser = ts_parser_new();
    ts_parser_set_language(ts_parser, tree_sitter_mx());

    TSTree *tree = ts_parser_parse_string(ts_parser, NULL, parser->src,
                                          strlen(parser->src));
    assert(tree != NULL);

    parser->tree = tree;

    TSNode root_node = ts_tree_root_node(tree);
    assert(!ts_node_is_null(root_node));

    walk_tree(parser, root_node);
}

Ast *parse(Arena *permanent_arena, const char *src) {
    MXParser parser = {0};
    mx_parser_init(permanent_arena, &parser, src);
    mx_parser_parse(&parser);

    Ast *ast = arena_alloc(permanent_arena, sizeof(Ast));
    ast->nodes = parser.nodes;
    ast->diagnostics = parser.diagnostics;
    return ast;
}
