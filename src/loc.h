#ifndef _MX_LOC_H
#define _MX_LOC_H

#include <stdint.h>

typedef struct {
    uint32_t row;
    uint32_t col;
} MXPosition;

typedef struct {
    MXPosition start;
    MXPosition end;
} MXRange;

#endif // _MX_LOC_H
