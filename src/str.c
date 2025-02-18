#include <string.h>

#include "str.h"

String *string_new(Arena *a, const char *data, size_t len) {
    String *s = arena_alloc_struct(a, String);
    s->data = data;
    s->len = len;
    return s;
}

String *string_from_cstr(Arena *a, const char *cstr) {
    return string_new(a, cstr, strlen(cstr));
}

const char *string_to_cstr(Arena *a, String *s) {
    char *cstr = arena_alloc(a, s->len + 1);
    memcpy(cstr, s->data, s->len);
    cstr[s->len] = '\0';
    return cstr;
}

String *string_concat(Arena *a, String *s1, String *s2) {
    char *data = arena_alloc(a, s1->len + s2->len);
    memcpy(data, s1->data, s1->len);
    memcpy(data + s1->len, s2->data, s2->len);
    return string_new(a, data, s1->len + s2->len);
}

bool string_eq(String *s1, String *s2) {
    if (s1->len != s2->len) {
        return false;
    }

    return memcmp(s1->data, s2->data, s1->len) == 0;
}

bool string_eq_cstr(String *s1, const char *s2) {
    size_t len = strlen(s2);
    if (s1->len != len) {
        return false;
    }

    return memcmp(s1->data, s2, len) == 0;
}
