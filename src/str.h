#ifndef _MX_STR_H
#define _MX_STR_H

#include <stddef.h>

#include "mem.h"

typedef struct {
    const char *data;
    size_t len;
} String;

String *string_new(Arena *a, const char *data, size_t len);

String *string_from_cstr(Arena *a, const char *cstr);

const char *string_to_cstr(Arena *a, String *s);

String *string_concat(Arena *a, String *s1, String *s2);

bool string_eq(String *s1, String *s2);

bool string_eq_cstr(String *s1, const char *s2);

#endif // _MX_STR_H
