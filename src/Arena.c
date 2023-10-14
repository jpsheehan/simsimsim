#include <stdio.h>
#include <stdlib.h>

#include "Arena.h"

Arena newArena(size_t size)
{
    Arena arena = {
        .size = size,
        .next = 0,
        .hwm = 0,
        .ptr = calloc(1, size)
    };

    if (arena.ptr == NULL) {
        arena.size = 0;
    }

    return arena;
}

void destroyArena(Arena* arena)
{
    free(arena->ptr);
    arena->ptr = NULL;
    arena->size = 0;
    arena->next = 0;
    arena->hwm = 0;
}

void* aalloc(Arena* arena, size_t size)
{
    void* ptr = arena->ptr + arena->next;
    arena->next += size;
    
    // printf("Allocate %ld\n", arena->next);
    if (arena->next > arena->size)
    {
        fprintf(stderr, "Could not allocate %ld bytes in arena.\n", arena->size);
        exit(1);
    }

    if (arena->next > arena->hwm) {
        arena->hwm = arena->next;
    }

    return ptr;
}

void resetArena(Arena* arena)
{
    arena->next = 0;
}