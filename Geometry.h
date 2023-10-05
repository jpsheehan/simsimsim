#ifndef Geometry_h
#define Geometry_h

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "Direction.h"

typedef struct
{
  int16_t x;
  int16_t y;
} Pos;


typedef struct
{
  uint16_t w;
  uint16_t h;
} Size;

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} Rect;

Pos addPos(Pos a, Pos b);
Pos moveInDirection(Pos pos, Direction dir);
bool isPosInAnyRect(Pos pos, Rect* rects, size_t rectCount);

#endif
