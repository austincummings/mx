#include <inttypes.h>
#include <tree_sitter/api.h>

#include "ts_ext.h"

void ts_node_print(Arena *a, const TSNode node, const char *src) {
    assert(a != NULL);
    assert(!ts_node_is_null(node));
    assert(src != NULL);
    // Print the node's type
    const char *type = ts_node_type(node);
    fprintf(stderr, "Node type: %s\n", type);
    fprintf(stderr, "    ID: %" PRIuPTR "\n", (uintptr_t)node.id);
    fprintf(stderr, "    Start: %d\n", ts_node_start_byte(node));
    fprintf(stderr, "    End: %d\n", ts_node_end_byte(node));
    fprintf(stderr, "    Children: %d\n", ts_node_child_count(node));
    fprintf(stderr, "    Named children: %d\n",
            ts_node_named_child_count(node));
    fprintf(stderr, "```mx\n");
    char *text = ts_node_text(a, node, src);
    fprintf(stderr, "%s\n", text);
    fprintf(stderr, "```\n");
}

char *ts_node_position(Arena *a, const TSNode node, const char *src) {
    assert(a != NULL);
    assert(!ts_node_is_null(node));
    assert(src != NULL);

    // Get the start and end positions of the node
    TSPoint start = ts_node_start_point(node);
    TSPoint end = ts_node_end_point(node);

    // Allocate space for the formatted position string
    size_t buffer_size = 100; // Sufficient for typical line:column formatting
    char *position_str = arena_alloc(a, buffer_size);

    // Format the position as a string
    snprintf(position_str, buffer_size, "%u:%u - %u:%u", start.row + 1,
             start.column + 1, end.row + 1, end.column + 1);

    return position_str;
}

char *ts_node_text(Arena *a, const TSNode node, const char *src) {
    assert(a != NULL);
    assert(!ts_node_is_null(node));
    assert(src != NULL);
    // Get the text of the node
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    uint32_t size = end - start;
    char *text = arena_alloc(a, size);
    memcpy(text, src + start, size);
    text[size] = '\0';
    return text;
}

char *ts_node_name(Arena *a, const TSNode node, const char *src) {
    TSNode name_node =
        ts_node_child_by_field_name(node, "name", strlen("name"));
    if (ts_node_is_null(name_node)) {
        return "";
    }
    return ts_node_text(a, name_node, src);
}

bool ts_node_matches(TSNode node, TSLanguage *language,
                     const char *query_string) {
    // Create a query
    uint32_t error_offset;
    TSQueryError error_type;
    TSQuery *query = ts_query_new(language, query_string, strlen(query_string),
                                  &error_offset, &error_type);

    // Check if query creation failed
    if (!query) {
        fprintf(stderr, "Failed to compile query at offset %d: error type %d\n",
                error_offset, error_type);
        return false;
    }

    // Set up a query cursor
    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, query, node);

    // Run the query to see if there's a match
    TSQueryMatch match = {0};
    bool has_match = ts_query_cursor_next_match(cursor, &match);

    // Clean up resources
    ts_query_cursor_delete(cursor);
    ts_query_delete(query);

    return has_match;
}

// Helper function to initialize a TSNodeList
void ts_node_list_init(Arena *a, TSNodeList *list, size_t initial_capacity) {
    list->nodes = (TSNode *)arena_alloc(a, initial_capacity * sizeof(TSNode));
    list->count = 0;
    list->capacity = initial_capacity;
}

// Helper function to add a node to TSNodeList
void ts_node_list_add(Arena *a, TSNodeList *list, TSNode node) {
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        TSNode *new_nodes =
            (TSNode *)arena_alloc(a, new_capacity * sizeof(TSNode));
        memcpy(new_nodes, list->nodes, list->count * sizeof(TSNode));
        list->nodes = new_nodes;
        list->capacity = new_capacity;
    }
    list->nodes[list->count++] = node;
}
