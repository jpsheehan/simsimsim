#include <stdlib.h>

#include "Direction.h"

Direction turnLeft(Direction dir) {
    return (Direction)((dir + (DIR_MAX - 1)) % DIR_MAX);
}

Direction turnRight(Direction dir) {
    return (Direction)((dir + 1) % DIR_MAX);
}

Direction turnBackwards(Direction dir) {
    return (Direction)((dir + (DIR_MAX >> 2)) % DIR_MAX);
}

Direction getRandomDirection(void) {
    return (Direction)(rand() % DIR_MAX);
}