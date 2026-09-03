#include "gc.h"

GC* gc_create(void) {
    GC *gc = (GC *)calloc(1, sizeof(GC));
    if (!gc) return NULL;
    
    gc->objects = NULL;
    gc->object_count = 0;
    gc->max_objects = 1000;
    gc->enabled = true;
    
    return gc;
}

void gc_destroy(GC *gc) {
    if (!gc) return;
    
    GCObject *current = gc->objects;
    while (current) {
        GCObject *next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
    
    free(gc);
}

void* gc_alloc(GC *gc, size_t size) {
    if (!gc || !gc->enabled) return malloc(size);
    
    // Verificar si es necesario hacer GC
    if (gc->object_count >= gc->max_objects) {
        gc_collect(gc);
    }
    
    GCObject *obj = (GCObject *)calloc(1, sizeof(GCObject));
    if (!obj) return NULL;
    
    obj->data = calloc(1, size);
    if (!obj->data) {
        free(obj);
        return NULL;
    }
    
    obj->size = size;
    obj->marked = false;
    obj->next = gc->objects;
    gc->objects = obj;
    gc->object_count++;
    
    return obj->data;
}

void gc_mark(GC *gc, void *ptr) {
    if (!gc || !ptr) return;
    
    GCObject *current = gc->objects;
    while (current) {
        if (current->data == ptr) {
            current->marked = true;
            break;
        }
        current = current->next;
    }
}

void gc_sweep(GC *gc) {
    if (!gc) return;
    
    GCObject **current = &gc->objects;
    while (*current) {
        if (!(*current)->marked) {
            GCObject *unmarked = *current;
            *current = unmarked->next;
            free(unmarked->data);
            free(unmarked);
            gc->object_count--;
        } else {
            (*current)->marked = false;
            current = &(*current)->next;
        }
    }
}

void gc_collect(GC *gc) {
    if (!gc || !gc->enabled) return;
    
    // Marcar todos los objetos accesibles
    // (En una implementación real, aquí se marcarían desde las raíces)
    
    // Barrer y liberar objetos no marcados
    gc_sweep(gc);
}

void gc_enable(GC *gc) {
    if (gc) gc->enabled = true;
}

void gc_disable(GC *gc) {
    if (gc) gc->enabled = false;
}
