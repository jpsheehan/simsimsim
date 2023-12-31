#ifndef Common_h
#define Common_h

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "SimFeatures.h"

#define IN_BASE 0x8000
#define OUT_BASE 0x4000
#define INTERNAL_BASE 0x200

#define MAX_NEURONS 128
#define MAX_CONNECTIONS 128

typedef enum {
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

typedef enum {
    IN_WORLD_X,
    IN_WORLD_Y,
    IN_AGE,
    IN_COLLIDE,
    IN_ENERGY,
    IN_VISION_FORWARD,
    IN_PROXIMITY_TO_NEAREST_EDGE,
    IN_PROXIMITY_TO_ORIGIN,
    IN_MAX
} InputType;

typedef enum {
    OUT_MOVE_X,
    OUT_MOVE_Y,
    OUT_MOVE_RANDOM,
    OUT_MOVE_FORWARD_BACKWARD,
    OUT_TURN_LEFT_RIGHT,
    OUT_TURN_RANDOM,
    OUT_MAX
} OutputType;

typedef struct {
    bool sourceIsInput : 1;
    uint8_t sourceId : 7;
    bool sinkIsOutput : 1;
    uint8_t sinkId : 7;
    uint16_t weight;
} Gene;

typedef struct {
    uint8_t count;
    Gene *genes;
} Genome;

struct Neuron_t;

typedef struct {
    uint16_t sourceId;
    uint16_t sinkId;
    float weight;
    bool visited;
} NeuralConnection;

typedef enum {
    NEURON_INPUT,
    NEURON_INTERNAL,
    NEURON_OUTPUT
} NeuronType;

typedef struct Neuron_t {
    uint16_t id;
    NeuronType type;
    float prevState;
    float state;
    uint8_t inputs;
    uint8_t inputsVisited;
    uint8_t outputs;
    uint8_t outputsVisited;
} Neuron;

typedef struct {
    uint16_t neuronCount;
    Neuron *neurons;
    uint16_t connectionCount;
    NeuralConnection *connections;
} NeuralNet;

typedef struct {
    int16_t x;
    int16_t y;
} Pos;

typedef struct {
    uint16_t w;
    uint16_t h;
} Size;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} Rect;

typedef uint16_t OrganismId;

typedef struct {
    OrganismId id;
    Pos pos;
    Genome genome;
    NeuralNet net;
    bool alive;
    bool didCollide;
    float energyLevel;
    Direction direction;
    OrganismId parentA;
    OrganismId parentB;
    bool mutated;
} Organism;

struct __simulation_t;

typedef struct {
    bool (*fn)(Organism*, struct __simulation_t*);
    const char* name;
} SelectionCriteria;

typedef struct __simulation_t {
    Size size;
    int seed;
    SelectionCriteria selector;
    Rect* obstacles;
    size_t obstaclesCount;
    float mutationRate;
    float energyToMove;
    float energyToRest;
    int maxInternalNeurons;
    int population;
    int stepsPerGeneration;
    int numberOfGenes;
    int maxGenerations;
} Simulation;

#if FEATURE_TRACE
#define TRACE(s) do {\
        printf("TRACE: %s\t%s (%s:%d)\n", s, __FUNCTION__, __FILE__, __LINE__);\
    }\
    while (0);
#define TRACE_BEGIN TRACE("Begin");
#define TRACE_END TRACE("End");
#else
#define TRACE(s)
#define TRACE_BEGIN
#define TRACE_END
#endif

#endif
