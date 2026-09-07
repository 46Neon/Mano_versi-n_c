#include "arena.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void){MilenaArena a;milena_arena_init(&a,32);int *x=milena_arena_alloc(&a,sizeof(*x),_Alignof(int));int *y=milena_arena_alloc(&a,sizeof(*y),_Alignof(int));assert(x&&y);*x=7;*y=9;assert(*x+*y==16);assert(a.allocations==2);milena_arena_reset(&a);assert(a.blocks==NULL);assert(milena_arena_alloc(&a,16,8));milena_arena_release(&a);puts("OK: Milena temporary arena");return 0;}
