#include <stdio.h>

#include "mem.h"

char *read_from_stdin(Arena *arena) {
    char stack_buffer[1024]; // Stack-allocated initial buffer
    size_t len = 0;
    size_t capacity = sizeof(stack_buffer);
    char *buffer = stack_buffer;

    int ch;
    while ((ch = getchar()) != EOF) {
        if (len + 1 >= capacity) {
            // Allocate in the arena when we exceed the stack buffer
            size_t new_capacity = capacity * 2;
            char *new_buffer = arena_alloc(arena, new_capacity);
            assert(new_buffer != NULL);

            // Copy existing data to arena buffer
            for (size_t i = 0; i < len; i++) {
                new_buffer[i] = buffer[i];
            }
            buffer = new_buffer;
            capacity = new_capacity;
        }
        buffer[len++] = (char)ch;
    }

    // Allocate final string in the arena and copy if needed
    char *final_buffer = arena_alloc(arena, len + 1);
    assert(final_buffer != NULL);

    for (size_t i = 0; i < len; i++) {
        final_buffer[i] = buffer[i];
    }
    final_buffer[len] = '\0'; // Null-terminate

    return final_buffer;
}
