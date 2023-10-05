#ifndef Direction_h
#define Direction_h

typedef enum
{
  DIR_N,
  DIR_NE,
  DIR_E,
  DIR_SE,
  DIR_S,
  DIR_SW,
  DIR_W,
  DIR_NW,
  DIR_MAX
} Direction;

Direction turnLeft(Direction dir);
Direction turnRight(Direction dir);
Direction turnBackwards(Direction dir);
Direction getRandomDirection(void);

#endif