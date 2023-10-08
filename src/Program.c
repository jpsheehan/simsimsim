#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "Common.h"
#include "Queue.h"
#include "Simulator.h"
#include "Selectors.h"
#include "Visualiser.h"

void* simWorker(void* args);
void* uiWorker(void* args);
extern sem_t simulatorReadyLock;
extern sem_t visualiserReadyLock;

int main(int argc, const char* argv[])
{
    Simulation sim;

    if (argc == 2) {
        if (sscanf(argv[1], "%d", &sim.seed) != 1) {
            fprintf(stderr, "Could not parse seed from argument.\n");
            sim.seed = time(NULL);
        }
    } else {
        sim.seed = time(NULL);
    }

    Rect obstacles[2] = {
        (Rect){.x = 32, .y = 48, .w = 2, .h = 32},
        (Rect){.x = 94, .y = 48, .w = 2, .h = 32},
    };

    sim.selector = &rightSelector;
    sim.mutationRate = 0.01;
    sim.obstacles = obstacles;
    sim.obstaclesCount = 0;
    sim.size = (Size) {
        .w = 128, .h = 128
    };
    sim.energyToMove = 0.01;
    sim.energyToRest = 0.01;
    sim.maxInternalNeurons = 2;
    sim.numberOfGenes = 4;
    sim.population = 1000;
    sim.stepsPerGeneration = 150;
    sim.maxGenerations = 1000;

    Queue simInbox, uiInbox;

    createQueue(&uiInbox, 200);
    createQueue(&simInbox, 200);

    SharedThreadState state = {
        .sim = &sim,
        .simInbox = &simInbox,
        .uiInbox = &uiInbox
    };

    sem_init(&simulatorReadyLock, 0, 0);
    sem_init(&visualiserReadyLock, 0, 0);

    pthread_t simThread, uiThread;

    pthread_create(&uiThread, NULL, uiWorker, &state);
    pthread_create(&simThread, NULL, simWorker, &state);

    pthread_join(uiThread, NULL);
    pthread_join(simThread, NULL);

    sem_destroy(&visualiserReadyLock);
    sem_destroy(&simulatorReadyLock);

    return EXIT_SUCCESS;
}

void* simWorker(void* args)
{
    SharedThreadState* state = (SharedThreadState*)args;
    runSimulation(state);
    return NULL;
}

void* uiWorker(void* args)
{
    SharedThreadState* state = (SharedThreadState*)args;
    runUserInterface(state);
    return NULL;
}
