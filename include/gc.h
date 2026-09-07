#ifndef MILENA_GC_H
#define MILENA_GC_H

#include "common.h"

typedef struct GCObject {
    struct GCObject *next;
    bool marked;
    size_t size;
    void *data;
} GCObject;

typedef struct GC {
    GCObject *objects;
    size_t object_count;
    size_t max_objects;
    bool enabled;
} GC;

GC* gc_create(void);
void gc_destroy(GC *gc);
void* gc_alloc(GC *gc, size_t size);
void gc_mark(GC *gc, void *ptr);
void gc_sweep(GC *gc);
void gc_collect(GC *gc);
void gc_enable(GC *gc);
void gc_disable(GC *gc);

#endif
