#ifndef TS_EXT_H
#define TS_EXT_H

#include <inttypes.h>
#include <stdio.h>
#include <tree_sitter/api.h>

#include "arena.h"
#include "map.h"

typedef struct TSNodeList {
    TSNode *nodes;
    size_t count;
    size_t capacity;
} TSNodeList;

void ts_node_print(Arena *a, TSNode node, const char *src);

char *ts_node_text(Arena *a, TSNode node, const char *src);

char *ts_node_id_to_string(Arena *a, uintptr_t node_id);

char *ts_node_name(Arena *a, TSNode node, const char *src);

bool ts_node_matches(TSNode node, TSLanguage *language,
                     const char *query_string);

/// Returns a HashMap[[c_str, TSNodeList*]]
HashMap *ts_node_query(Arena *a, TSNode node, const TSLanguage *language,
                       const char *query_string);

TSNodeList *ts_query_nodes(HashMap *query_results, char *capture_name);

TSNode ts_query_first_node(HashMap *query_results, char *capture_name);

bool ts_query_has_capture(HashMap *query_results, char *capture_name);

#endif
