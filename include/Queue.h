#ifndef Queue_h
#define Queue_h

#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    sem_t lock;
    int16_t size;
    int16_t wHead;
    int16_t rHead;
    bool wHeadChasing;
    void** items;
} Queue;

void createQueue(Queue* queue, int16_t size);
void destroyQueue(Queue* queue);

bool enqueue(Queue* queue, void* item);
void* dequeue(Queue* queue);

#endif
