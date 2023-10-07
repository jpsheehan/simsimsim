#include <bits/time.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Common.h"
#include "Simulator.h"
#include "Visualiser.h"
#include "Geometry.h"
#include "Organism.h"

static volatile bool interrupted = false;

void signalHandler(int sig)
{
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
    Organism **orgsByPosition = calloc(sim->size.w * sim->size.h, sizeof(Organism*));

    visSetObstacles(sim->obstacles, sim->obstaclesCount);

    for (int i = 0; i < sim->population; i++) {
        orgs[i] = makeRandomOrganism(sim, orgsByPosition);
    }

    float Ao10Buffer[10] = {0.0f};
    int Ao10Idx = 0;
    uint64_t lastTimeInMicroseconds;

    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    lastTimeInMicroseconds = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

    for (int g = 0; g < sim->maxGenerations; g++) {
        visSetGeneration(g);

        for (int step = 0; step < sim->stepsPerGeneration; step++) {
            memset(orgsByPosition, 0, sim->size.h * sim->size.w * sizeof(Organism*));
            visSetStep(step);
            visDrawStep(orgs, sim->population, false);

            for (int i = 0; i < sim->population; i++) {
                organismRunStep(&orgs[i], orgsByPosition, sim, step);
            }

            if (interrupted || visGetWantsToQuit())
                break;
        }

        if (interrupted || visGetWantsToQuit()) {
            break;
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

        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t endOfStepMicroseconds = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
        uint64_t diff = endOfStepMicroseconds - lastTimeInMicroseconds;
        lastTimeInMicroseconds = endOfStepMicroseconds;

        double generationsPerMinute = 60000000.0 / diff;

        printf("Gen %d survival rate is %d/%d (%03.2f%%, %03.2f%% Ao10) with %d "
               "dead before and "
               "%d dead after selection. Took %ld us. %.2f Gen/min.\n",
               g, survivors, sim->population, survivalRate, Ao10,
               deadBeforeSelection, deadAfterSelection, diff, generationsPerMinute);

        if (survivors <= 1) {
            break;
        }

        for (int i = 0; i < sim->population; i++) {
            Organism *a, *b;
            findMates(orgs, sim->population, &a, &b);
            nextGenOrgs[i] = makeOffspring(a, b, sim, orgsByPosition);
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
    free(orgsByPosition);
}
