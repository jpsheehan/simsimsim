#ifndef Geometry_h
#define Geometry_h

#include "Common.h"

Pos addPos(Pos a, Pos b);
Pos moveInDirection(Pos pos, Direction dir);
bool isPosInAnyRect(Pos pos, Rect* rects, size_t rectCount);

#endif
