#ifndef visualiser_h
#define visualiser_h

#include <stdint.h>
#include <SDL2/SDL.h>
#include "sim.h"

void visInit(uint32_t w, uint32_t h);
void visDestroy(void);

void visDrawBlank(void);

void visSetGeneration(int);
void visSetStep(int);
void visDrawStep(Organism* orgs, uint32_t count);

#endif