#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>

#include "Features.h"

#if FEATURE_VISUALISER
#include <pthread.h>
#include <semaphore.h>
#endif

#include "Common.h"
#include "Simulator.h"
#include "Selectors.h"
#include "Visualiser.h"

void* simWorker(void* args);

#if FEATURE_VISUALISER
void* uiWorker(void* args);
extern sem_t simulatorReadyLock;
extern sem_t visualiserReadyLock;
#endif

int main(int argc, const char* argv[])
{
    setlocale(LC_NUMERIC, "");

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

    sim.selector = circleCenterSelector;
    sim.mutationRate = 0.01;
    sim.obstacles = obstacles;
    sim.obstaclesCount = 0;
    sim.size = (Size) {
        .w = 128, .h = 128
    };
    sim.energyToMove = 0.01;
    sim.energyToRest = 0.01;
    sim.maxInternalNeurons = 8;
    sim.numberOfGenes = 32;
    sim.population = 1000;
    sim.stepsPerGeneration = 150;
    sim.maxGenerations = 20000;

#if FEATURE_VISUALISER
    sem_init(&simulatorReadyLock, 0, 0);
    sem_init(&visualiserReadyLock, 0, 0);

    pthread_t simThread, uiThread;

    pthread_create(&uiThread, NULL, uiWorker, &sim);
    pthread_create(&simThread, NULL, simWorker, &sim);

    pthread_join(uiThread, NULL);
    pthread_join(simThread, NULL);

    sem_destroy(&visualiserReadyLock);
    sem_destroy(&simulatorReadyLock);
#else
    runSimulation(&sim);
#endif
    return EXIT_SUCCESS;
}

#if FEATURE_VISUALISER
void* simWorker(void* args)
{
    Simulation* sim = (Simulation*)args;
    runSimulation(sim);
    return NULL;
}

void* uiWorker(void* args)
{
    Simulation* sim = (Simulation*)args;
    runUserInterface(sim);
    return NULL;
}
#endif