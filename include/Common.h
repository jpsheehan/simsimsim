#ifndef Common_h
#define Common_h

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define IN_BASE 0x8000
#define OUT_BASE 0x4000
#define INTERNAL_BASE 0x200
#define FEATURE_VISUALISER true

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

typedef struct {
    uint16_t id;
    Pos pos;
    Genome genome;
    NeuralNet net;
    bool alive;
    bool didCollide;
    float energyLevel;
    Direction direction;
} Organism;

typedef struct __simulation_t {
    Size size;
    int seed;
    bool (*selector)(Organism*, struct __simulation_t*);
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

#endif
