#ifndef ARENA_H
#define ARENA_H

#define ARENA_DEFAULT_RESERVE_SIZE 64000000000

#include "assert.h"
#include "errno.h"
#include "memory.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "unistd.h"

typedef struct Arena {
    uint64_t reserved;
    uint64_t committed;
    uint64_t used;
    void *start;
    void *next;
} Arena;

/// Initializes a new arena.
void arena_init(Arena *a, uint64_t size);

/// Frees the arena.
void arena_release(Arena *a);

/// Pushes a block of memory of the given size to the arena.
void *arena_alloc(Arena *a, uint64_t size);

#define arena_alloc_struct(a, type) (type *)arena_alloc(a, sizeof(type))

char *arena_strcat(Arena *a, char *str0, char *str1);

void arena_dump(Arena *a);

#endif
