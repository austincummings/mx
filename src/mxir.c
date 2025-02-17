#include "mxir.h"
#include "array_list.h"
#include "mem.h"

Mxir *mxir_new(Arena *a) {
    Mxir *ir = arena_alloc_struct(a, Mxir);
    arraylist_init(a, &ir->nodes, 24);
    arraylist_init(a, &ir->diagnostics, 24);
    return ir;
}
