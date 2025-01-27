#include "map.h"

static uint32_t hash(const char *key) {
    uint32_t hash = 2166136261U; // FNV offset basis
    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= 16777619; // FNV prime
    }
    return hash % TABLE_SIZE;
}

HashMap *hashmap_init(Arena *a) {
    HashMap *hashmap = arena_alloc(a, sizeof(HashMap));
    hashmap->table = arena_alloc(a, sizeof(HashNode *) * TABLE_SIZE);
    hashmap->size = 0;
    for (int i = 0; i < TABLE_SIZE; i++) {
        hashmap->table[i] = NULL;
    }
    return hashmap;
}

void hashmap_set(Arena *a, HashMap *hashmap, const char *key, void *value) {
    uint32_t index = hash(key);
    HashNode *new_node = arena_alloc(a, sizeof(HashNode));
    new_node->key = arena_alloc(a, strlen(key) + 1);
    strcpy(new_node->key, key);
    new_node->value = value;
    new_node->next = hashmap->table[index];
    hashmap->size++;
    hashmap->table[index] = new_node;
}

void *hashmap_get(HashMap *hashmap, const char *key) {
    uint32_t index = hash(key);
    HashNode *node = hashmap->table[index];
    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }
    return NULL; // Key not found
}

HashMapIterator hashmap_iterator(HashMap *hashmap) {
    HashMapIterator it;
    it.hashmap = hashmap;
    it.bucket_index = 0;
    it.current_node = NULL;

    // Find the first non-empty bucket
    while (it.bucket_index < TABLE_SIZE &&
           hashmap->table[it.bucket_index] == NULL) {
        it.bucket_index++;
    }
    if (it.bucket_index < TABLE_SIZE) {
        it.current_node = hashmap->table[it.bucket_index];
    }
    return it;
}

int hashmap_iterator_has_next(HashMapIterator *it) {
    return it->current_node != NULL;
}

void hashmap_iterator_next(HashMapIterator *it, const char **key,
                           void **value) {
    if (it->current_node != NULL) {
        *key = it->current_node->key;
        *value = it->current_node->value;

        // Move to the next node in the current bucket
        it->current_node = it->current_node->next;

        // If the current node is NULL, move to the next bucket
        while (it->current_node == NULL && ++it->bucket_index < TABLE_SIZE) {
            it->current_node = it->hashmap->table[it->bucket_index];
        }
    }
}

// Example usage of the iterator
void hashmap_print_all(HashMap *hashmap) {
    HashMapIterator it = hashmap_iterator(hashmap);
    const char *key;
    void *value;
    while (hashmap_iterator_has_next(&it)) {
        hashmap_iterator_next(&it, &key, &value);
        fprintf(stderr, "Key: %s, Value: %p\n", key, value);
    }
}
