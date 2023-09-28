#ifndef sim_h
#define sim_h

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
  IN_WORLD_X,
  IN_WORLD_Y,
  IN_MAX
} InputType;

typedef enum
{
  OUT_MOVE_X,
  OUT_MOVE_Y,
  OUT_MOVE_RANDOM,
  OUT_MAX
} OutputType;


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
  bool sourceIsInput : 1;
  uint8_t sourceId : 7;
  bool sinkIsOutput : 1;
  uint8_t sinkId : 7;
  uint16_t weight;
} Gene;

typedef struct
{
  uint8_t count;
  Gene *genes;
} Genome;

struct Neuron_t;

typedef struct
{
  uint16_t sourceId;
  uint16_t sinkId;
  float weight;
  bool visited;
} NeuralConnection;

typedef enum
{
  NEURON_INPUT,
  NEURON_INTERNAL,
  NEURON_OUTPUT
} NeuronType;

typedef struct Neuron_t
{
  uint16_t id;
  NeuronType type;
  float prevState;
  float state;
  uint8_t inputs;
  uint8_t inputsVisited;
  uint8_t outputs;
  uint8_t outputsVisited;
} Neuron;

typedef struct
{
  uint16_t neuronCount;
  Neuron *neurons;
  uint16_t connectionCount;
  NeuralConnection *connections;
} NeuralNet;

typedef struct
{
  Pos pos;
  Genome genome;
  NeuralNet net;
  bool alive;
} Organism;

typedef struct
{
  uint16_t step;
  Organism *organisms;
} Generation;

typedef struct
{
  Size size;
} Simulation;

#endif