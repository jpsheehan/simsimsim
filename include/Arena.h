#ifndef Arena_h
#define Arena_h

#include <stddef.h>

typedef struct {
    void* ptr;
    size_t next;
    size_t size;
    size_t hwm;
} Arena;

Arena newArena(size_t size);
void destroyArena(Arena* arena);
void* aalloc(Arena* arena, size_t size);
void resetArena(Arena* arena);

#endif
