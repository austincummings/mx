#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include <stddef.h>
#include <stdint.h>

#include "arena.h"

#define ArrayList(T)                                                           \
    struct {                                                                   \
        T *data;                                                               \
        size_t capacity;                                                       \
        size_t size;                                                           \
    }

/// Initializes an ArrayList with a given capacity.
#define arraylist_init(arena, list, initial_capacity)                          \
    do {                                                                       \
        (list)->capacity = (initial_capacity);                                 \
        (list)->size = 0;                                                      \
        (list)->data =                                                         \
            arena_alloc((arena), sizeof(*(list)->data) * (list)->capacity);    \
    } while (0)

/// Adds an item to the ArrayList.
#define arraylist_add(arena, list, item)                                       \
    do {                                                                       \
        if ((list)->size >= (list)->capacity) {                                \
            size_t new_capacity = (list)->capacity * 2;                        \
            void *new_data =                                                   \
                arena_alloc((arena), sizeof(*(list)->data) * new_capacity);    \
            memcpy(new_data, (list)->data,                                     \
                   sizeof(*(list)->data) * (list)->size);                      \
            (list)->data = new_data;                                           \
            (list)->capacity = new_capacity;                                   \
        }                                                                      \
        (list)->data[(list)->size++] = (item);                                 \
    } while (0)

/// Gets an item from the ArrayList by index.
#define arraylist_get(list, index)                                             \
    (((size_t)(index) < (list)->size) ? &(list)->data[(index)] : NULL)

/// Clears the ArrayList.
#define arraylist_clear(list)                                                  \
    do {                                                                       \
        (list)->size = 0;                                                      \
    } while (0)

/// Frees resources associated with the ArrayList (does not free arena memory).
#define arraylist_release(list)                                                \
    do {                                                                       \
        (list)->data = NULL;                                                   \
        (list)->capacity = 0;                                                  \
        (list)->size = 0;                                                      \
    } while (0)

#endif // ARRAY_LIST_H
