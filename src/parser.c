#include <stdio.h>
#include <string.h>
#include <tree_sitter/api.h>

#include "ast.h"
#include "debug.h"
#include "mem.h"
#include "parser.h"

static const char *node_text(MXParser *parser, TSNode node) {
    const char *text =
        arena_alloc(parser->permanent_arena,
                    ts_node_end_byte(node) - ts_node_start_byte(node) + 1);
    memcpy((void *)text, parser->src + ts_node_start_byte(node),
           ts_node_end_byte(node) - ts_node_start_byte(node));
    return text;
}

static const char *node_type(MXParser *parser, TSNode node) {
    const char *type_orig = ts_node_type(node);
    const char *type =
        arena_alloc(parser->permanent_arena, strlen(type_orig) + 1);
    memcpy((void *)type, type_orig, strlen(type_orig) + 1);
    return type;
}

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

    const char *type = node_type(parser, node);
    const char *text = node_text(parser, node);

    // Store any syntax errors
    if (ts_node_is_error(node)) {
        MXDiagnostic diag = {
            .range = range,
            .kind = MX_DIAG_SYNTAX_ERROR,
        };
        arraylist_add(parser->permanent_arena, &parser->diagnostics, diag);
    }

    // TODO: Refactor out extra syntax checks
    if (strcmp(type, "block") == 0) {
        // Check that the block has a "end" field
        TSNode end_node =
            ts_node_child_by_field_name(node, "end", strlen("end"));
        assert(!ts_node_is_null(end_node));
        const char *text = node_text(parser, end_node);

        if (strcmp(text, "}") != 0) {
            MXDiagnostic diag = {
                .range = range,
                .kind = MX_DIAG_SYNTAX_ERROR_EXPECTED_END_BRACE,
            };
            arraylist_add(parser->permanent_arena, &parser->diagnostics, diag);
        }
    }

    if (strcmp(type, "break_stmt") == 0 || strcmp(type, "continue_stmt") == 0 ||
        strcmp(type, "return_stmt") == 0 || strcmp(type, "assign_stmt") == 0 ||
        strcmp(type, "expr_stmt") == 0) {
        // Check that the node has a "semi" field that is a semicolon
        TSNode end_node =
            ts_node_child_by_field_name(node, "semi", strlen("semi"));
        assert(!ts_node_is_null(end_node));
        const char *text = node_text(parser, end_node);

        if (strcmp(text, ";") != 0) {
            MXDiagnostic diag = {
                .range = range,
                .kind = MX_DIAG_SYNTAX_ERROR_EXPECTED_SEMICOLON,
            };
            arraylist_add(parser->permanent_arena, &parser->diagnostics, diag);
        }
    }

    AstNodeRef self_ref = parser->nodes.size;

    // Store the node
    AstNode ast_node = {
        .self_ref = self_ref, .type = type, .text = text, .range = range};
    ast_node.children =
        arena_alloc_struct(parser->permanent_arena, AstNodeRefList);
    arraylist_init(parser->permanent_arena, ast_node.children, 24);
    ast_node.named_children = hashmap_init(parser->permanent_arena);
    arraylist_add(parser->permanent_arena, &parser->nodes, ast_node);

    for (size_t i = 0; i < ts_node_named_child_count(node); i++) {
        TSNode child = ts_node_named_child(node, i);
        AstNodeRef child_ref = walk_tree(parser, child);
        arraylist_add(parser->permanent_arena, ast_node.children, child_ref);

        // Get the name of the child
        const char *name = ts_node_field_name_for_named_child(node, i);
        if (name != NULL) {
            hashmap_set(parser->permanent_arena, ast_node.named_children, name,
                        (void *)child_ref);
        }
    }

    arena_release(&scratch);

    fflush(stderr);

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
