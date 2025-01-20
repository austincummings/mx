#ifndef MAP_H
#define MAP_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"

#define TABLE_SIZE 1024

typedef struct HashNode {
    char *key;
    void *value;
    struct HashNode *next;
} HashNode;

typedef struct HashMap {
    HashNode **table;
    size_t size;
} HashMap;

typedef struct {
    HashMap *hashmap;
    int bucket_index;
    HashNode *current_node;
} HashMapIterator;

HashMap *hashmap_init(Arena *a);

void hashmap_insert(Arena *a, HashMap *hashmap, const char *key, void *value);

void *hashmap_get(HashMap *hashmap, const char *key);

HashMapIterator hashmap_iterator(HashMap *hashmap);

int hashmap_iterator_has_next(HashMapIterator *it);

void hashmap_iterator_next(HashMapIterator *it, const char **key, void **value);

void hashmap_print_all(HashMap *hashmap);

#endif // MAP_H
