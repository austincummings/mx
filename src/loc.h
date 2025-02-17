#ifndef _MX_LOC_H
#define _MX_LOC_H

#include <stdint.h>

#include "mem.h"

typedef struct {
    uint32_t row;
    uint32_t col;
} MXPosition;

typedef struct {
    MXPosition start;
    MXPosition end;
} MXRange;

const char *mx_range_to_string(Arena *a, MXRange range);

#endif // _MX_LOC_H
