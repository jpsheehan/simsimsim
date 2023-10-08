#include "Queue.h"
#include <semaphore.h>
#include <stdlib.h>

void createQueue(Queue* queue, int16_t size)
{
    queue->size = size;
    queue->items = calloc(size, sizeof(void*));
    if (!queue->items) return;
    sem_init(&queue->lock, 0, 1);
    queue->wHead = 0;
    queue->rHead = 0;
    queue->wHeadChasing = false;
}

void destroyQueue(Queue* queue)
{
    sem_destroy(&queue->lock);
    free(queue->items);
    queue->items = NULL;
}

bool enqueue(Queue* queue, void* item)
{
    sem_wait(&queue->lock);

    if (queue->rHead == queue->wHead && queue->wHeadChasing) {
        sem_post(&queue->lock);
        return false;
    }

    queue->items[queue->wHead++] = item;
    if (queue->wHead >= queue->size) {
        queue->wHead = 0;
        queue->wHeadChasing = true;
    }

    sem_post(&queue->lock);

    return true;
}

void* dequeue(Queue* queue)
{
    sem_wait(&queue->lock);

    if (queue->rHead == queue->wHead && !queue->wHeadChasing) {
        sem_post(&queue->lock);
        return NULL;
    }

    void* item = queue->items[queue->rHead++];
    if (queue->rHead == queue->size) {
        queue->rHead = 0;
        queue->wHeadChasing = false;
    }

    sem_post(&queue->lock);

    return item;
}