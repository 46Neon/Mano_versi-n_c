#include "arena.h"

bool arena_init(Arena *arena, size_t capacity) {
    if (!arena || capacity == 0) return false;
    
    arena->data = (unsigned char *)malloc(capacity);
    if (!arena->data) return false;
    
    arena->capacity = capacity;
    arena->used = 0;
    arena->next = NULL;
    return true;
}

void *arena_alloc(Arena *arena, size_t size) {
    if (!arena || !arena->data || size == 0) return NULL;
    
    size_t alignment = _Alignof(max_align_t);
    size_t padding = (alignment - (arena->used % alignment)) % alignment;
    
    if (arena->used + padding + size > arena->capacity) {
        // Intentar expandir
        if (arena->next) {
            return arena_alloc(arena->next, size);
        }
        
        // Crear nuevo bloque
        Arena *new_arena = (Arena *)malloc(sizeof(Arena));
        if (!new_arena) return NULL;
        
        size_t new_capacity = arena->capacity * 2;
        if (new_capacity < size) new_capacity = size;
        
        if (!arena_init(new_arena, new_capacity)) {
            free(new_arena);
            return NULL;
        }
        
        arena->next = new_arena;
        return arena_alloc(new_arena, size);
    }
    
    size_t start = arena->used + padding;
    arena->used = start + size;
    memset(arena->data + start, 0, size);
    return (void *)(arena->data + start);
}

void *arena_realloc(Arena *arena, void *ptr, size_t old_size, size_t new_size) {
    if (!ptr) return arena_alloc(arena, new_size);
    if (new_size <= old_size) return ptr;
    
    void *new_ptr = arena_alloc(arena, new_size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, old_size);
    }
    return new_ptr;
}

void arena_destroy(Arena *arena) {
    if (!arena) return;
    
    if (arena->next) {
        arena_destroy(arena->next);
        free(arena->next);
        arena->next = NULL;
    }
    
    free(arena->data);
    arena->data = NULL;
    arena->capacity = 0;
    arena->used = 0;
}

void arena_reset(Arena *arena) {
    if (!arena) return;
    
    arena->used = 0;
    if (arena->next) {
        arena_reset(arena->next);
    }
}

size_t arena_used(const Arena *arena) {
    if (!arena) return 0;
    size_t total = arena->used;
    if (arena->next) {
        total += arena_used(arena->next);
    }
    return total;
}

size_t arena_capacity(const Arena *arena) {
    if (!arena) return 0;
    size_t total = arena->capacity;
    if (arena->next) {
        total += arena_capacity(arena->next);
    }
    return total;
}

char *arena_strdup(Arena *arena, const char *str) {
    if (!arena || !str) return NULL;
    size_t len = strlen(str) + 1;
    char *copy = (char *)arena_alloc(arena, len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}
