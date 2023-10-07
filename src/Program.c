#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "Simulator.h"
#include "Selectors.h"

int main(int argc, const char* argv[])
{
    Simulation sim;

    if (argc == 2)
    {
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

    sim.selector = &centerSelector;
    sim.mutationRate = 0.01;
    sim.obstacles = obstacles;
    sim.obstaclesCount = 0;
    sim.size = (Size) {
        .w = 128, .h = 128
    };
    sim.energyToMove = 0.01;
    sim.energyToRest = 0.01;
    sim.maxInternalNeurons = 1;
    sim.population = 1000;
    sim.stepsPerGeneration = 150;
    sim.numberOfGenes = 2;
    sim.maxGenerations = 10000;

    runSimulation(&sim);

    return EXIT_SUCCESS;
}