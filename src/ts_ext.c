#include "ts_ext.h"
#include "array_list.h"
#include "map.h"
#include <stdint.h>
#include <tree_sitter/api.h>

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

/// Returns the text of the line that the node is on
char *ts_node_line_text(Arena *a, TSNode node, const char *src) {
    assert(a != NULL);
    assert(!ts_node_is_null(node));
    assert(src != NULL);
    // Find the start of the line
    uint32_t start = ts_node_start_byte(node);
    while (start > 0 && src[start - 1] != '\n') {
        start--;
    }
    // Find the end of the line
    uint32_t end = ts_node_start_byte(node);
    while (src[end] != '\0' && src[end] != '\n') {
        end++;
    }
    // Calculate the size of the line
    uint32_t size = end - start;
    // Allocate memory in the arena for the line
    char *line = arena_alloc(a, size);
    // Copy the line into the buffer
    memcpy(line, src + start, size);
    line[size] = '\0';
    return line;
}

const char *ptr_to_str(Arena *a, const void *node_id) {
    // Pre-calculate the size needed for the string representation
    size_t s = snprintf(NULL, 0, "%" PRIuPTR, (uintptr_t)node_id) + 1;

    // Allocate memory in the arena for the string representation
    char *buffer = (char *)arena_alloc(a, s);
    if (buffer == NULL) {
        return NULL; // Allocation failed
    }

    // Convert the uintptr_t node_id to a string
    snprintf(buffer, s, "%" PRIuPTR, (uintptr_t)node_id);

    return buffer;
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

HashMap *ts_node_query(Arena *a, TSNode node, const TSLanguage *language,
                       const char *query_string, bool direct_children_only) {
    // Create the TSQuery from the query string
    uint32_t error_offset;
    TSQueryError error_type;
    TSQuery *query = ts_query_new(language, query_string, strlen(query_string),
                                  &error_offset, &error_type);
    if (!query) {
        fprintf(stderr,
                "Failed to create TSQuery: error at offset %u, error type %d\n",
                error_offset, error_type);
        fprintf(stderr, "Query:\n%s\n", query_string);
        return NULL;
    }

    // Create a query cursor
    TSQueryCursor *cursor = ts_query_cursor_new();

    // Initialize the hash map using the arena allocator
    HashMap *capture_map = hashmap_init(a);

    // Iterate through matches found by the query cursor
    ts_query_cursor_exec(cursor, query, node);
    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match)) {
        // fprintf(stderr, "match capture count: %d\n", match.capture_count);
        for (uint32_t i = 0; i < match.capture_count; i++) {
            // Extract information from each capture
            TSQueryCapture capture = match.captures[i];
            TSNode captured_node = capture.node;
            uint32_t capture_index = capture.index;
            uint32_t len = 0;
            const char *capture_name =
                ts_query_capture_name_for_id(query, capture_index, &len);
            // fprintf(stderr, "capture name: %s\n", capture_name);

            // Check if the capture index is already in the map
            TSNodeList *node_list =
                (TSNodeList *)hashmap_get(capture_map, capture_name);
            if (!node_list) {
                // If not present, create a new TSNodeList
                node_list = (TSNodeList *)arena_alloc(a, sizeof(TSNodeList));
                arraylist_init(a, node_list, 10);
                hashmap_set(a, capture_map, capture_name, (void *)node_list);
            }

            // Add the captured node to the list
            if (direct_children_only) {
                if (ts_node_parent(captured_node).id == node.id) {
                    arraylist_add(a, node_list, captured_node);
                }
            } else {
                arraylist_add(a, node_list, captured_node);
            }
        }
    }

    // Clean up TSQuery and query cursor after use
    ts_query_cursor_delete(cursor);
    ts_query_delete(query);

    assert(capture_map != NULL);

    return capture_map;
}

TSNodeList *ts_query_nodes(HashMap *query_results, char *capture_name) {
    assert(query_results != NULL);
    assert(capture_name != NULL);

    TSNodeList *nodes = hashmap_get(query_results, capture_name);
    return nodes;
}

TSNode ts_query_first_node(HashMap *query_results, char *capture_name) {
    assert(query_results != NULL);
    assert(capture_name != NULL);

    TSNodeList *list = ts_query_nodes(query_results, capture_name);
    if (list == NULL) {
        fprintf(stderr, "Could not find capture in query results: %s\n",
                capture_name);
        return (TSNode){0};
    }
    TSNode node = list->data[0];
    return node;
}

bool ts_query_has_capture(HashMap *query_results, char *capture_name) {
    assert(query_results != NULL);
    assert(capture_name != NULL);
    return hashmap_get(query_results, capture_name) != NULL;
}

const TSLanguage *tree_sitter_mx(void);

HashMap *query(Arena *a, TSNode node, const char *query_string,
               bool direct_children_only) {
    return ts_node_query(a, node, tree_sitter_mx(), query_string,
                         direct_children_only);
}
