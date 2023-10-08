#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

#include "Common.h"
#include "Simulator.h"
#include "Visualiser.h"
#include "Geometry.h"
#include "Organism.h"

static volatile bool interrupted = false;
static sem_t paused;
static sem_t framePaused;
sem_t simulatorReadyLock;

void signalHandler(int sig)
{
    if (sig == SIGINT) {
        interrupted = true;
        sem_post(&paused);
        sem_post(&framePaused);
        // printf("Interrupt sent\n");
    }
}

void simSendQuit(void)
{
    interrupted = true;
    sem_post(&paused);
    sem_post(&framePaused);
    // printf("Quit sent\n");
}

void simSendReady(void)
{
    sem_post(&simulatorReadyLock);
}

void simSendPause(void) {
    sem_wait(&paused);
}
void simSendContinue(void) {
    sem_post(&paused);
}

void simSendFramePause(void) {
    sem_wait(&framePaused);
}

void simSendFrameContinue(void) {
    sem_post(&framePaused);
}

void runSimulation(SharedThreadState *sharedThreadState)
{
    Simulation *sim = sharedThreadState->sim;
    sem_init(&paused, 0, 1);
    sem_init(&framePaused, 0, 0);

    signal(SIGINT, &signalHandler);

    srand(sim->seed);
    printf("Seed is %d\n", sim->seed);

    Organism *orgs = calloc(sim->population, sizeof(Organism));
    Organism *nextGenOrgs = calloc(sim->population, sizeof(Organism));
    Organism **orgsByPosition = calloc(sim->size.w * sim->size.h, sizeof(Organism*));
    Organism **prevOrgsByPosition = calloc(sim->size.w * sim->size.h, sizeof(Organism*));

    for (int i = 0; i < sim->population; i++) {
        orgs[i] = makeRandomOrganism(sim, orgsByPosition);
        orgs[i].id = i;
    }

    float Ao10Buffer[10] = {0.0f};
    int Ao10Idx = 0;
    uint64_t lastTimeInMicroseconds;

    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    lastTimeInMicroseconds = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

    sem_wait(&simulatorReadyLock);
    if (interrupted) goto quitOuterLoop;
    visSendReady();

    for (int g = 0; g < sim->maxGenerations; g++) {
        visSendGeneration(orgs, g);

        sem_wait(&paused);
        sem_post(&paused);
        if (interrupted) goto quitOuterLoop;

        sem_wait(&framePaused);
        sem_post(&framePaused);
        if (interrupted) goto quitOuterLoop;

        for (int step = 0; step < sim->stepsPerGeneration; step++) {
            memcpy(prevOrgsByPosition, orgsByPosition, sim->size.h * sim->size.w * sizeof(Organism*));
            memset(orgsByPosition, 0, sim->size.h * sim->size.w * sizeof(Organism*));

            visSendStep(orgs, step);

            sem_wait(&paused);
            sem_post(&paused);
            if (interrupted) goto quitOuterLoop;

            sem_wait(&framePaused);
            sem_post(&framePaused);
            if (interrupted) goto quitOuterLoop;

            for (int i = 0; i < sim->population; i++) {
                organismRunStep(&orgs[i], orgsByPosition, prevOrgsByPosition, sim, step);
            }
            // int fpVal, pVal;
            // sem_getvalue(&paused, &pVal);
            // sem_getvalue(&framePaused, &fpVal);
            // printf("Finished calculating step %d (paused = %d, framePaused = %d)\n", step, pVal, fpVal);
            
            if (interrupted) goto quitOuterLoop;
        }

        if (interrupted) goto quitOuterLoop;


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

        visSendStep(orgs, sim->stepsPerGeneration);

        sem_wait(&paused);
        sem_post(&paused);
        if (interrupted) goto quitOuterLoop;

        sem_wait(&framePaused);
        sem_post(&framePaused);
        if (interrupted) goto quitOuterLoop;

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
        double kiloStepsPerMinute = sim->stepsPerGeneration * 60000.0 / diff;

        printf("Gen %d survival rate is %d/%d (%03.2f%%, %03.2f%% Ao10) with %d "
               "dead before and "
               "%d dead after selection. %.2f kSteps/min. %.2f Gen/min.\n",
               g, survivors, sim->population, survivalRate, Ao10,
               deadBeforeSelection, deadAfterSelection, kiloStepsPerMinute, generationsPerMinute);

        if (survivors <= 1) {
            break;
        }

        for (int i = 0; i < sim->population; i++) {
            Organism *a, *b;
            findMates(orgs, sim->population, &a, &b);
            nextGenOrgs[i] = makeOffspring(a, b, sim, orgsByPosition);
            nextGenOrgs[i].id = i;
        }

        for (int i = 0; i < sim->population; i++) {
            destroyOrganism(&orgs[i]);
        }

        Organism* tmp = orgs;
        orgs = nextGenOrgs;
        nextGenOrgs = tmp;

        if (interrupted || survivors <= 1)
            goto quitOuterLoop;

    }

quitOuterLoop:
    visSendQuit();

    for (int i = 0; i < sim->population; i++) {
        destroyOrganism(&orgs[i]);
    }

    sem_destroy(&paused);
    sem_destroy(&framePaused);
    free(orgs);
    free(nextGenOrgs);
    free(orgsByPosition);
    free(prevOrgsByPosition);
}
