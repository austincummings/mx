#include "emitter.h"

MXEmitter *mx_emitter_new(Arena *arena) {
    MXEmitter *emitter = arena_alloc_struct(arena, MXEmitter);
    emitter->out = NULL;
    return emitter;
}
