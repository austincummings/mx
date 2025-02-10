#ifndef _MX_SERVER_H
#define _MX_SERVER_H

#include "map.h"
typedef struct {
    Arena *permanent_arena;
    Arena arena;

    bool running;

    HashMap *documents;
} MXLangServer;

void mx_lang_server_init(Arena *permanent_arena, MXLangServer *server);

void mx_lang_server_listen(MXLangServer *server);

#endif // _MX_SERVER_H
