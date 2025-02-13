#ifndef _MX_CODEGEN_H
#define _MX_CODEGEN_H

#include "mem.h"

typedef struct {
    char *src;
} MXCodeGen;

MXCodeGen *mx_codegen_new(Arena *arena);

#endif // _MX_CODEGEN_H
