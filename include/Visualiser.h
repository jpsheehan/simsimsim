#ifndef Visualiser_h
#define Visualiser_h

#include <stdint.h>
#include <SDL2/SDL.h>
#include "Common.h"
#include "Arena.h"

void visSendGeneration(Arena* arena, Organism* orgs, int generation);
void visSendStep(Arena* arena, Organism* orgs, int step);
void visSendQuit(void);
void visSendReady(void);
void visSendDisconnected(void);

void runUserInterface(Simulation*);

#endif