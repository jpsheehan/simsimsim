#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Simulator.h"
#include "Visualiser.h"
#include "Geometry.h"
#include "Organism.h"

static volatile bool interrupted = false;

void signalHandler(int sig) {
    interrupted = true;
}

void runSimulation(Simulation *sim)
{
    visInit(sim->size.w, sim->size.h);
    signal(SIGINT, &signalHandler);

    srand(sim->seed);
    printf("Seed is %d\n", sim->seed);
    visSetSeed(sim->seed);

    Organism *orgs = calloc(sim->population, sizeof(Organism));
    Organism *nextGenOrgs = calloc(sim->population, sizeof(Organism));

    visSetObstacles(sim->obstacles, sim->obstaclesCount);

    for (int i = 0; i < sim->population; i++) {
        orgs[i] = makeRandomOrganism(sim->numberOfGenes, sim, orgs, i);
    }

    float Ao10Buffer[10] = {0.0f};
    int Ao10Idx = 0;

    for (int g = 0; g < sim->maxGenerations; g++) {
        visSetGeneration(g);

        for (int step = 0; step < sim->stepsPerGeneration; step++) {

            visSetStep(step);
            visDrawStep(orgs, sim->population, false);

            for (int i = 0; i < sim->population; i++) {
                organismRunStep(&orgs[i], orgs, sim, i, step, sim->stepsPerGeneration);
            }

            if (interrupted || visGetWantsToQuit())
                break;
        }

        if (interrupted || visGetWantsToQuit()) {
            break;
        }

        // assert that we don't have two organisms occupying the same cell
        int occupiedCells = 0;
        for (int i = 0; i < sim->population; i++) {
            if (!orgs[i].alive)
                continue;
            // printf("Org #%d has pos (%d, %d) at end of gen %d\n", i, orgs[i].pos.x,
            //        orgs[i].pos.y, g);
            if (getOrganismByPos(orgs[i].pos, orgs, i, true)) {
                occupiedCells++;
            }
        }
        if (occupiedCells) {
            printf("Found %d occupied cells\n", occupiedCells);
        }

        int survivors = 0;
        int deadBeforeSelection = 0;
        int deadAfterSelection = 0;
        for (int i = 0; i < sim->population; i++) {
            Organism *org = &orgs[i];

            if (!org->alive) {
                deadBeforeSelection++;
                continue;
            }

            if (sim->selector(org, sim)) {
                survivors++;
            } else {
                deadAfterSelection++;
                org->alive = false;
            }
        }

        visDrawStep(orgs, sim->population, true);

        float survivalRate = (float)survivors * 100.0f / sim->population;

        Ao10Buffer[Ao10Idx] = survivalRate;
        Ao10Idx = (Ao10Idx + 1) % 10;

        float Ao10 = 0.0f;
        for (int a = 0; a < 10; a++) {
            Ao10 += Ao10Buffer[a];
        }
        Ao10 /= (float)(g < 10 ? g + 1 : 10);

        printf("Gen %d survival rate is %d/%d (%03.2f%%, %03.2f%% Ao10) with %d "
               "dead before and "
               "%d dead after selection.\n",
               g, survivors, sim->population, survivalRate, Ao10,
               deadBeforeSelection, deadAfterSelection);

        if (survivors <= 1) {
            break;
        }

        for (int i = 0; i < sim->population; i++) {
            Organism *a, *b;
            findMates(orgs, sim->population, &a, &b);
            nextGenOrgs[i] = makeOffspring(a, b, sim, orgs, i);
        }

        for (int i = 0; i < sim->population; i++) {
            destroyOrganism(&orgs[i]);
        }

        Organism* tmp = orgs;
        orgs = nextGenOrgs;
        nextGenOrgs = tmp;

        if (interrupted || survivors <= 1 || visGetWantsToQuit())
            break;
    }

    for (int i = 0; i < sim->population; i++) {
        destroyOrganism(&orgs[i]);
    }

    visDestroy();

    free(orgs);
    free(nextGenOrgs);
}
