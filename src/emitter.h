#ifndef _MX_EMITTER_H
#define _MX_EMITTER_H

#include "mem.h"

typedef struct {
    char *out;
} MXEmitter;

MXEmitter *mx_emitter_new(Arena *arena);

#endif // _MX_EMITTER_H
