#include "arena.h"
#include <assert.h>
#include <stdio.h>
int main(void){Arena arena;assert(arena_init(&arena,64));int *x=arena_alloc(&arena,sizeof(*x));assert(x);*x=7;assert(*x==7);assert(arena_used(&arena)>=sizeof(*x));arena_reset(&arena);assert(arena_used(&arena)==0);arena_destroy(&arena);puts("OK: Milena temporary arena");return 0;}
