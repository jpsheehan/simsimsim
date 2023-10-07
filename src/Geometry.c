#include "Geometry.h"

Pos addPos(Pos a, Pos b) {
    return (Pos) {
        .x = a.x + b.x, .y = a.y + b.y
    };
}

Pos moveInDirection(Pos pos, Direction dir) {
    Pos lookup[DIR_MAX] = {
        (Pos){.x = 0, .y = -1},  // N
        (Pos){.x = 1, .y = -1},  // NE
        (Pos){.x = 1, .y = 0},   // E
        (Pos){.x = 1, .y = 1},   // SE
        (Pos){.x = 0, .y = 1},   // S
        (Pos){.x = -1, .y = 1},  // SW
        (Pos){.x = -1, .y = 0},  // W
        (Pos){.x = -1, .y = -1}, // NW
    };

    return addPos(pos, lookup[dir]);
}

bool isPosInRect(Pos pos, Rect rect) {
    return pos.x >= rect.x && pos.x < rect.x + rect.w + 1 && pos.y >= rect.y &&
           pos.y < rect.y + rect.h + 1;
}

bool isPosInAnyRect(Pos pos, Rect* rects, size_t rectCount) {
    for (size_t i = 0; i < rectCount; i++) {
        if (isPosInRect(pos, rects[i])) {
            return true;
        }
    }
    return false;
}