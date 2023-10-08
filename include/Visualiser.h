#ifndef Visualiser_h
#define Visualiser_h

#include <stdint.h>
#include <SDL2/SDL.h>
#include "Common.h"

void visSendGeneration(Organism* orgs, int generation);
void visSendStep(Organism* orgs, int step);
void visSendQuit(void);
void visSendReady(void);

void runUserInterface(Simulation*);

#endif