#include "arena.h"

/// Allocates a new arena.
void arena_init(Arena *arena, size_t size) {
    assert(arena != NULL);
    assert(size > 0);

    arena->reserved = size;
    arena->committed = 0;
    arena->start =
        mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    arena->next = arena->start;
}

/// Frees the arena.
void arena_release(Arena *arena) {
    assert(arena->reserved > 0);
    assert(arena->start != NULL);
    assert(arena->next != NULL);

    munmap(arena->start, arena->reserved);
    arena->committed = 0;
    arena->reserved = 0;
    arena->start = NULL;
    arena->next = NULL;
}

/// Pushes a block of memory of the given size to the arena.
void *arena_alloc(Arena *arena, size_t size) {
    assert(size > 0);
    assert(arena->reserved > 0);
    assert(arena->start != NULL);
    assert(arena->next != NULL);

    // Align size to page boundaries
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t aligned_size = (size + page_size - 1) & ~(page_size - 1);

    assert(aligned_size <= arena->reserved);

    uint8_t *arena_end = (uint8_t *)arena->start + arena->reserved;
    uint8_t *next_end = (uint8_t *)arena->next + aligned_size;

    if (next_end > arena_end) {
        fprintf(stderr, "Overflowing arena\n");
        abort();
    }

    // Commit the memory
    // printf("Committing %zu bytes\n", aligned_size);
    if (mprotect(arena->next, aligned_size, PROT_READ | PROT_WRITE) == -1) {
        char *err = strerror(errno);
        fprintf(stderr, "Could not commit arena bytes: %s\n", err);
        abort();
    }
    uint8_t *result = arena->next;
    memset(result, 0, aligned_size);
    arena->next = next_end;
    arena->committed += aligned_size;
    return result;
}

char *arena_strcat(Arena *a, char *str0, char *str1) {
    // Calculate the lengths of the input strings
    size_t len0 = strlen(str0);
    size_t len1 = strlen(str1);

    // Calculate the total length required, including null terminator
    size_t total_len = len0 + len1 + 1;

    // Allocate memory within the arena
    char *result = (char *)arena_alloc(a, total_len);
    if (!result) {
        return NULL; // Allocation failed
    }

    // Copy str0 to the allocated memory
    memcpy(result, str0, len0);

    // Append str1 to the result
    memcpy(result + len0, str1, len1);

    // Null-terminate the result
    result[total_len - 1] = '\0';

    return result;
}

void arena_dump(Arena *arena) {
    assert(arena != NULL);

    fprintf(stderr, "Arena: %p\n", (void *)arena);
    fprintf(stderr, "Committed: %lu\n", arena->committed);
    fprintf(stderr, "Reserved: %lu\n", arena->reserved);
    fprintf(stderr, "Start: %p\n", (void *)arena->start);
    fprintf(stderr, "End: %p\n",
            (void *)((uint8_t *)arena->start + arena->reserved));
    fprintf(stderr, "Next: %p\n", (void *)arena->next);

    // Print out each byte in blocks of 16.
    for (size_t i = 0;
         i < (size_t)((uint8_t *)arena->next - (uint8_t *)arena->start); i++) {
        if (i % 16 == 0) {
            fprintf(stderr, "\n%p: ", (void *)((uint8_t *)arena->start + i));
        }
        fprintf(stderr, "%02x ", ((uint8_t *)arena->start)[i]);
    }
    fprintf(stderr, "\n");
}
