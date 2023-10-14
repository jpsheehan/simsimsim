#ifndef LineGraph_h
#define LineGraph_h

#include <SDL2/SDL.h>
#include <stddef.h>
#include "Common.h"

typedef struct {
    size_t max;
    size_t count;
    SDL_Point* points;
    float* values;
    SDL_Color lineColor;
    Pos pos;
    Size size;
} LineGraph;

LineGraph createLineGraph(size_t maxElements, SDL_Color lineColor, Pos pos, Size size);
void destroyLineGraph(LineGraph*);
void renderLineGraph(LineGraph*, SDL_Renderer*);
void setPointOnGraph(LineGraph*, size_t, float);

#endif