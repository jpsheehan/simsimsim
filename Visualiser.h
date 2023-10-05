#ifndef Visualiser_h
#define Visualiser_h

#include <stdint.h>
#include <SDL2/SDL.h>
#include "Simulator.h"

void visInit(uint32_t w, uint32_t h);
void visDestroy(void);

void visDrawBlank(void);

void visSetGeneration(int);
void visSetStep(int);
void visSetSeed(int);
void visDrawStep(Organism* orgs, uint32_t count, bool forceDraw);
void visSetObstacles(Rect* obstacles, int count);
bool visGetWantsToQuit(void);

#endif