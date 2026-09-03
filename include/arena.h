#ifndef MANO_ARENA_H
#define MANO_ARENA_H

#include "common.h"

typedef struct Arena {
    unsigned char *data;
    size_t capacity;
    size_t used;
    struct Arena *next;
} Arena;

bool arena_init(Arena *arena, size_t capacity);
void *arena_alloc(Arena *arena, size_t size);
void *arena_realloc(Arena *arena, void *ptr, size_t old_size, size_t new_size);
void arena_destroy(Arena *arena);
void arena_reset(Arena *arena);
size_t arena_used(const Arena *arena);
size_t arena_capacity(const Arena *arena);
char *arena_strdup(Arena *arena, const char *str);

#endif
