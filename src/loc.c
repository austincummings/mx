#include "loc.h"
#include <stdio.h>

const char *mx_range_to_string(Arena *a, MXRange range) {
    assert(a != NULL);

    char *buf = arena_alloc(a, 64);
    snprintf(buf, 64, "%d:%d-%d:%d", range.start.row, range.start.col,
             range.end.row, range.end.col);
    return buf;
}
