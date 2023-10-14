#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

#include "Features.h"
#include "Common.h"
#include "Simulator.h"
#include "Visualiser.h"
#include "Geometry.h"
#include "Organism.h"
#include "Arena.h"

static volatile bool interrupted = false;
#if FEATURE_VISUALISER
static sem_t paused;
static sem_t framePaused;
sem_t simulatorReadyLock;
#endif

void signalHandler(int sig)
{
    if (sig == SIGINT) {
        interrupted = true;
#if FEATURE_VISUALISER
        sem_post(&paused);
        sem_post(&framePaused);
#endif
        // printf("Interrupt sent\n");
    }
}

void simSendQuit(void)
{
    interrupted = true;
#if FEATURE_VISUALISER
    sem_post(&paused);
    sem_post(&framePaused);
#endif
    // printf("Quit sent\n");
}

void simSendReady(void)
{
#if FEATURE_VISUALISER
    sem_post(&simulatorReadyLock);
    // printf("simSendReady() #unlocked\n");
#endif
}

void simSendPause(void)
{
    if (interrupted) return;
#if FEATURE_VISUALISER
    sem_wait(&paused);
#endif
}
void simSendContinue(void)
{
    if (interrupted) return;
#if FEATURE_VISUALISER
    sem_post(&paused);
#endif
}

void simSendFramePause(void)
{
    if (interrupted) return;
#if FEATURE_VISUALISER
    sem_wait(&framePaused);
#endif
}

void simSendFrameContinue(void)
{
    if (interrupted) return;
#if FEATURE_VISUALISER
    sem_post(&framePaused);
#endif
}

void runSimulation(Simulation *s)
{
    Simulation *sim = s;
#if FEATURE_VISUALISER
    sem_init(&paused, 0, 1);
    sem_init(&framePaused, 0, 0);
#endif

    signal(SIGINT, &signalHandler);

    srand(sim->seed);
    printf("Seed is %d\n", sim->seed);

    Arena arena = newArena(1024 * 1024 * 100);

    Organism *orgs = aalloc(&arena, sim->population * sizeof(Organism));
    Organism *nextGenOrgs = aalloc(&arena, sim->population * sizeof(Organism));
    Organism **orgsByPosition = aalloc(&arena, sim->size.w * sim->size.h * sizeof(Organism*));
    Organism **prevOrgsByPosition = aalloc(&arena, sim->size.w * sim->size.h * sizeof(Organism*));

    for (int i = 0; i < sim->population; i++) {
        orgs[i] = makeRandomOrganism(&arena, sim, orgsByPosition);
        orgs[i].id = i;
    }

    float Ao10Buffer[10] = {0.0f};
    int Ao10Idx = 0;
    uint64_t lastTimeInMicroseconds;

    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    lastTimeInMicroseconds = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

#if FEATURE_VISUALISER
    sem_wait(&simulatorReadyLock);
    if (interrupted) goto quitOuterLoop;
#endif
    visSendReady();

    Arena generationArena = newArena(1024 * 1024 * 100);

    for (int g = 0; g < sim->maxGenerations; g++) {
        visSendGeneration(&generationArena, orgs, g);

#if FEATURE_VISUALISER
        sem_wait(&paused);
        sem_post(&paused);
        if (interrupted) goto quitOuterLoop;
#endif

#if FEATURE_VISUALISER
        sem_wait(&framePaused);
        sem_post(&framePaused);
        if (interrupted) goto quitOuterLoop;
#endif

        for (int step = 0; step < sim->stepsPerGeneration; step++) {
            memcpy(prevOrgsByPosition, orgsByPosition, sim->size.h * sim->size.w * sizeof(Organism*));
            memset(orgsByPosition, 0, sim->size.h * sim->size.w * sizeof(Organism*));

            visSendStep(&generationArena, orgs, step);

#if FEATURE_VISUALISER
            sem_wait(&paused);
            sem_post(&paused);
            if (interrupted) goto quitOuterLoop;
#endif

#if FEATURE_VISUALISER
            sem_wait(&framePaused);
            sem_post(&framePaused);
            if (interrupted) goto quitOuterLoop;
#endif

            for (int i = 0; i < sim->population; i++) {
                organismRunStep(&orgs[i], orgsByPosition, prevOrgsByPosition, sim, step);
            }

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

            if (sim->selector.fn(org, sim)) {
                survivors++;
            } else {
                deadAfterSelection++;
                org->alive = false;
            }
        }

        visSendStep(&generationArena, orgs, sim->stepsPerGeneration - 1);

#if FEATURE_VISUALISER
        sem_wait(&paused);
        sem_post(&paused);
        if (interrupted) goto quitOuterLoop;
#endif

#if FEATURE_VISUALISER
        sem_wait(&framePaused);
        sem_post(&framePaused);
        if (interrupted) goto quitOuterLoop;
#endif

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
            nextGenOrgs[i] = makeOffspring(&generationArena, a, b, sim, orgsByPosition);
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

        resetArena(&generationArena);
    }

quitOuterLoop:
    if (interrupted) {
        visSendQuit();
    } else {
        visSendDisconnected();
    }

    for (int i = 0; i < sim->population; i++) {
        destroyOrganism(&orgs[i]);
    }

#if FEATURE_VISUALISER
    sem_destroy(&paused);
    sem_destroy(&framePaused);
#endif

    destroyArena(&generationArena);
    destroyArena(&arena);
}
