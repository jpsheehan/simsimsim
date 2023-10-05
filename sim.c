#include "sim.h"
#include "visualiser.h"
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define IN_BASE 0x8000
#define OUT_BASE 0x4000
#define INTERNAL_BASE 0x200

#define SIM_INTERNAL_MAX 3
#define SIM_WIDTH 128
#define SIM_HEIGHT 128
#define SIM_GEN_STEPS 300
#define SIM_NUM_GENES 8
#define SIM_POPULATION 1000
#define SIM_MAX_GENERATIONS 1000
#define SIM_MUTATION_RATE 0.01f
#define SIM_ENERGY_TO_MOVE 0.01f
#define SIM_ENERGY_TO_REST 0.01f

#define SIM_COLLISION_DEATHS false

static Rect obstacles[2] = {
    (Rect){.x = 32, .y = 48, .w = 2, .h = 32},
    (Rect){.x = 94, .y = 48, .w = 2, .h = 32},
};
static int obstaclesCount = 0;

static char *InputTypeStrings[IN_MAX] = {"WORLD_X",
                                         "WORLD_Y",
                                         "IN_AGE",
                                         "IN_COLLIDE",
                                         "IN_ENERGY",
                                         "VISION_FORWARD",
                                         "IN_PROXIMITY_TO_NEAREST_EDGE"};

static char *OutputTypeStrings[OUT_MAX] = {
    "MOVE_X",          "MOVE_Y",     "MOVE_RANDOM", "MOVE_FORWARD_BACKWARD",
    "TURN_LEFT_RIGHT", "TURN_RANDOM"};

Direction turnLeft(Direction dir) {
  return (Direction)((dir + (DIR_MAX - 1)) % DIR_MAX);
}

Direction turnRight(Direction dir) { return (Direction)((dir + 1) % DIR_MAX); }

Direction turnBackwards(Direction dir) {
  return (Direction)((dir + (DIR_MAX >> 2)) % DIR_MAX);
}

Direction getRandomDirection(void) { return (Direction)(rand() % DIR_MAX); }

Pos addPos(Pos a, Pos b) { return (Pos){.x = a.x + b.x, .y = a.y + b.y}; }

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

Gene intToGene(uint32_t n) {
  return (Gene){
      .sourceIsInput = (bool)((n >> 31) & 0x01),
      .sourceId = (uint8_t)((n >> 24) & 0x7f),
      .sinkIsOutput = (bool)((n >> 23) & 0x01),
      .sinkId = (uint8_t)((n >> 16) & 0x7f),
      .weight = (uint16_t)(n & 0xffff),
  };
}

uint32_t geneToInt(Gene *gene) {
  return (uint32_t)(((gene->sourceIsInput << 7) | gene->sourceId) << 24) |
         (uint32_t)(((gene->sinkIsOutput << 7) | gene->sinkId) << 16) |
         (uint32_t)gene->weight;
}

void testGeneCreation() {
  uint32_t geneInt = rand() % UINT32_MAX;

  Gene gene = intToGene(geneInt);

  uint32_t newGeneInt = geneToInt(&gene);

  if (geneInt != newGeneInt) {
    fprintf(stderr, "ERROR: %08X /= %08X\n", geneInt, newGeneInt);
    exit(1);
  }
}

char *geneToString(Gene *gene) {
  char *buffer = calloc(9, sizeof(char));
  snprintf(buffer, 9, "%08X", geneToInt(gene));
  return buffer;
}

char *genomeToString(Genome *genome) {
  size_t size = 9 * genome->count + 1;
  char *buffer = calloc(size, sizeof(char));
  for (int i = 0; i < genome->count; i++) {
    char *geneBuffer = geneToString(&genome->genes[i]);
    sprintf(&buffer[i * 9], "%s", geneBuffer);
    if (i + 1 != genome->count) {
      buffer[i * 9 + 8] = ' ';
    }
    free(geneBuffer);
  }
  return buffer;
}

Neuron *findNeuronById(Neuron *neurons, size_t n, uint16_t id) {
  for (int i = 0; i < n; i++) {
    if (neurons[i].id == id) {
      return &neurons[i];
    }
  }
  return NULL;
}

void organismBuildNeuralNet(Organism *org) {
  Neuron neurons[128] = {0};
  uint8_t usedNeurons = 0;

  NeuralConnection connections[128] = {0};
  uint8_t usedConnections = 0;

  for (int i = 0; i < org->genome.count; i++) {
    Gene *gene = &org->genome.genes[i];

    uint16_t sourceId = gene->sourceId;
    if (gene->sourceIsInput) {
      sourceId %= IN_MAX;
      sourceId |= IN_BASE;
    } else {
      sourceId %= SIM_INTERNAL_MAX;
      sourceId |= INTERNAL_BASE;
    }

    uint16_t sinkId = gene->sinkId;
    if (gene->sinkIsOutput) {
      sinkId %= OUT_MAX;
      sinkId |= OUT_BASE;
    } else {
      sinkId %= SIM_INTERNAL_MAX;
      sinkId |= INTERNAL_BASE;
    }

    Neuron *source = findNeuronById(neurons, usedNeurons, sourceId);
    if (source == NULL) {
      source = &neurons[usedNeurons++];
      source->id = sourceId;
      source->type = gene->sourceIsInput ? NEURON_INPUT : NEURON_INTERNAL;
      source->state = 0.0f;
      source->inputs = 0;
      source->outputs = 1;
      source->inputsVisited = 0;
      source->outputsVisited = 0;
    } else {
      source->outputs++;
    }

    Neuron *sink = findNeuronById(neurons, usedNeurons, sinkId);
    if (sink == NULL) {
      sink = &neurons[usedNeurons++];
      sink->id = sinkId;
      sink->type = gene->sinkIsOutput ? NEURON_OUTPUT : NEURON_INTERNAL;
      sink->state = 0.0f;
      sink->inputs = 1;
      sink->outputs = 0;
      sink->inputsVisited = 0;
      sink->outputsVisited = 0;
    } else {
      sink->inputs++;
    }

    // printf("Source for neuron %d has id %d\n", i, source->id);
    // printf("Sink for neuron %d has id %d\n", i, sink->id);

    NeuralConnection *conn = &connections[usedConnections++];
    conn->sourceId = source->id;
    conn->sinkId = sink->id;
    conn->weight =
        (float)gene->weight * 8.0f / 65536.0f - 4.0f; // between -4.0 and +4.0
    conn->visited = false;
  }

  org->net.connectionCount = usedConnections;
  org->net.connections = calloc(usedConnections, sizeof(NeuralConnection));
  memcpy(org->net.connections, connections,
         usedConnections * sizeof(NeuralConnection));

  org->net.neuronCount = usedNeurons;
  org->net.neurons = calloc(usedNeurons, sizeof(Neuron));
  memcpy(org->net.neurons, neurons, usedNeurons * sizeof(Neuron));

  // Neuron *source = findNeuronById(org->net.neurons, org->net.neuronCount,
  // 32768); printf("Neuron 0 has %d inputs and %d outputs\n", source->inputs,
  // source->outputs); printf("Made net with %d connections between %d
  // neurons\n", org->net.connectionCount, org->net.neuronCount);

  // Neuron *sink = findNeuronById(org->net.neurons, org->net.neuronCount,
  // 16384); printf("Net sink is %p, source is %p\n", sink, source);
}

bool isPosInRect(Pos pos, Rect rect) {
  return pos.x >= rect.x && pos.x < rect.x + rect.w + 1 && pos.y >= rect.y &&
         pos.y < rect.y + rect.h + 1;
}

bool isPosInObstacle(Pos pos) {
  for (int i = 0; i < obstaclesCount; i++) {
    if (isPosInRect(pos, obstacles[i])) {
      return true;
    }
  }
  return false;
}

void organismDestroyNeuralNet(Organism *org) {
  free(org->net.connections);
  free(org->net.neurons);
  org->net.connections = NULL;
  org->net.neurons = NULL;
  org->net.connectionCount = 0;
  org->net.neuronCount = 0;
}

Organism *getOrganismByPos(Pos pos, Organism *orgs, int orgsCount,
                           bool aliveOnly) {
  for (int i = 0; i < orgsCount; i++) {
    Organism *org = &orgs[i];

    if (pos.x == org->pos.x && pos.y == org->pos.y) {
      if ((aliveOnly && org->alive) || !aliveOnly)
        return org;
    }
  }
  return NULL;
}

void organismMoveBackIntoZone(Organism *org) {
  // Pos original = org->pos;
  if (org->pos.x >= SIM_WIDTH) {
    org->pos.x = SIM_WIDTH - 1;
  } else if (org->pos.x < 0) {
    org->pos.x = 0;
  }
  if (org->pos.y >= SIM_HEIGHT) {
    org->pos.y = SIM_HEIGHT - 1;
  } else if (org->pos.y < 0) {
    org->pos.y = 0;
  }
  // if (org->pos.x != original.x || org->pos.y != original.y) {
  //   printf("Moved back into zone: (%d, %d)\n", org->pos.x, org->pos.y);
  // }
}

void organismRunStep(Organism *org, Organism *otherOrgs, int otherOrgsCount,
                     int currentStep, int maxStep) {

  if (!org->alive)
    return;

  Pos originalPosition = org->pos;

  // reset
  for (int i = 0; i < org->net.connectionCount; i++) {
    org->net.connections[i].visited = false;
    org->didCollide = false;
  }
  for (int i = 0; i < org->net.neuronCount; i++) {
    org->net.neurons[i].inputsVisited = 0;
    org->net.neurons[i].outputsVisited = 0;
    org->net.neurons[i].prevState = org->net.neurons[i].state;
    org->net.neurons[i].state = 0.0f;
  }

  // excite
  for (int i = 0; i < org->net.neuronCount; i++) {
    Neuron *input = &org->net.neurons[i];
    if (input->type != NEURON_INPUT) {
      continue;
    }

    switch (input->id & 0xff) {
    case IN_WORLD_X:
      input->state = (float)org->pos.x / (float)SIM_WIDTH;
      break;
    case IN_WORLD_Y:
      input->state = (float)org->pos.y / (float)SIM_HEIGHT;
      break;
    case IN_AGE:
      input->state = (float)currentStep / (float)maxStep;
      break;
    case IN_COLLIDE:
      input->state = org->didCollide ? 1.0 : 0.0;
      break;
    case IN_ENERGY:
      input->state = org->energyLevel;
      break;
    case IN_VISION_FORWARD:
      // TODO: this won't work since we need to split the organism run step into
      // two stages... the inputs should operate on the old state and the
      // outputs should operate on the new state
      if (getOrganismByPos(
              addPos(org->pos, moveInDirection(org->pos, org->direction)),
              otherOrgs, otherOrgsCount, true)) {
        input->state = 1.0f;
      } else {
        input->state = 0.0f;
      }
      break;
    case IN_PROXIMITY_TO_NEAREST_EDGE: {
      int nearX = SIM_WIDTH / 2 - abs(SIM_WIDTH / 2 - org->pos.x);
      int nearY = SIM_HEIGHT / 2 - abs(SIM_HEIGHT / 2 - org->pos.y);
      input->state = (nearX < nearY ? 1.0f - 2.0f * (float)nearX / SIM_WIDTH
                                    : 1.0f - 2.0f * (float)nearY / SIM_HEIGHT);
    } break;
    }

    if (fabs(input->state) > 1.0f)
      printf("Excited neuron #%d to level %.2f\n", input->id, input->state);
  }

  // compute
  int visits;
  do {
    visits = 0;

    for (int i = 0; i < org->net.connectionCount; i++) {
      NeuralConnection *connection = &org->net.connections[i];

      Neuron *source = findNeuronById(org->net.neurons, org->net.neuronCount,
                                      connection->sourceId);
      Neuron *sink = findNeuronById(org->net.neurons, org->net.neuronCount,
                                    connection->sinkId);

      // if the source has inputs and they haven't been fulfilled then don't do
      // anything here yet
      if (source->inputs && (source->inputsVisited < source->inputs)) {
        continue;
      }

      // if the connection has already been visited then skip
      if (connection->visited) {
        continue;
      }

      if (sink == source) {
        sink->state += connection->weight * source->prevState;
      } else {
        sink->state += connection->weight * source->state;
      }

      sink->inputsVisited++;
      source->outputsVisited++;
      connection->visited = true;
      // printf("Visited connection between neuron #%d and neuron #%d\n",
      // source->id, sink->id);

      // normalise the sink state if it has been completed between -1.0 and 1.0.
      if (sink->inputsVisited == sink->inputs) {
        sink->state = tanhf(sink->state / (float)sink->inputs);
        // printf("Normalised neuron #%d state to %.2f\n", sink->id,
        // sink->state);
      }

      visits++;
      // printf("Source has %d of %d inputs fulfilled\n", source->inputsVisited,
      // source->inputs);
    }
  } while (visits > 0);

  // act
  bool didMove = false;
  for (int i = 0; i < org->net.neuronCount; i++) {
    Neuron *output = &org->net.neurons[i];

    if (output->type != NEURON_OUTPUT) {
      // printf("Neuron #%d is not an output\n", output->id);
      continue;
    }

    // printf("Output neuron #%d has level %.2f\n", output->id, output->state);

    switch (output->id & 0xff) {
    case OUT_MOVE_X:
      if (output->state >= 0.5f) {
        org->pos.x++;
        didMove = true;
      } else if (output->state <= -0.5f) {
        org->pos.x--;
        didMove = true;
      }
      break;
    case OUT_MOVE_Y:
      if (output->state >= 0.5f) {
        org->pos.y++;
        didMove = true;
      } else if (output->state <= -0.5f) {
        org->pos.y--;
        didMove = true;
      }
      break;
    case OUT_MOVE_RANDOM:
      if (fabs(output->state) >= 0.5f) {
        org->pos.x += rand() % 2 == 0 ? -1 : 1;
        org->pos.y += rand() % 2 == 0 ? -1 : 1;
        didMove = true;
      }
      break;
    case OUT_MOVE_FORWARD_BACKWARD:
      if (output->state >= 0.5f) {
        org->pos = moveInDirection(org->pos, org->direction);
        didMove = true;
      } else if (fabs(output->state) <= 0.5f) {
        org->pos = moveInDirection(org->pos, turnBackwards(org->direction));
        didMove = true;
      }
      break;
    case OUT_TURN_LEFT_RIGHT:
      if (output->state <= 0.5f) {
        org->direction = turnLeft(org->direction);
      } else if (output->state >= 0.5f) {
        org->direction = turnRight(org->direction);
      }
      break;
    case OUT_TURN_RANDOM:
      if (fabs(output->state) >= 0.5f) {
        org->direction =
            rand() % 2 ? turnLeft(org->direction) : turnRight(org->direction);
      }
      break;
    }
  }

  if (didMove) {
    org->energyLevel -= SIM_ENERGY_TO_MOVE;
  } else {
    org->energyLevel += SIM_ENERGY_TO_REST;
  }

  if (org->energyLevel <= 0.0f) {
    org->alive = false;
    org->energyLevel = 0.0f;
    org->pos = originalPosition;
    return;
  } else if (org->energyLevel > 1.0f) {
    org->energyLevel = 1.0f;
  }

  // collisions
  organismMoveBackIntoZone(org);

#if SIM_COLLISION_DEATHS
  if (getOrganismByPos(org->pos, otherOrgs, otherOrgsCount, true) ||
      isPosInObstacle(org->pos)) {
    org->didCollide = true;
    org->pos = originalPosition;
    if (getOrganismByPos(org->pos, otherOrgs, otherOrgsCount, true) ||
        isPosInObstacle(org->pos)) {
      org->alive = false;
      return;
    }
  }
#else
  while (getOrganismByPos(org->pos, otherOrgs, otherOrgsCount, true) ||
         isPosInObstacle(org->pos)) {
    org->didCollide = true;
    org->pos.x += rand() % 3 - 1;
    org->pos.y += rand() % 3 - 1;
    organismMoveBackIntoZone(org);
  }
#endif
}

uint32_t rand_uint32(void) {
  return (uint32_t)((uint16_t)(rand()) << 16) | (uint16_t)rand();
}

Genome makeRandomGenome(uint8_t numGenes) {
  Genome genome = {.count = numGenes, .genes = calloc(numGenes, sizeof(Gene))};

  for (int i = 0; i < numGenes; i++) {
    genome.genes[i] = intToGene(rand_uint32());
  }

  return genome;
}

Organism makeRandomOrganism(uint8_t numGenes, int worldWidth, int worldHeight,
                            Organism *otherOrgs, int otherOrgsCount) {
  Organism org = {
      .pos = (Pos){.x = rand() % worldWidth, .y = rand() % worldHeight},
      .genome = makeRandomGenome(numGenes),
      .alive = true,
      .didCollide = false,
      .energyLevel = 1.0,
      .direction = getRandomDirection()};

  while (getOrganismByPos(org.pos, otherOrgs, otherOrgsCount, false) ||
         isPosInObstacle(org.pos)) {
    org.pos.x = rand() % worldWidth;
    org.pos.y = rand() % worldWidth;
  }

  organismBuildNeuralNet(&org);

  return org;
}

Genome reproduce(Genome *a, Genome *b);

Genome mutate(Genome genome, float mutationRate) {
  if ((float)rand() / (float)RAND_MAX >= mutationRate) {
    return genome;
  }

  // flip a random bit
  int idx = rand() % genome.count;
  int geneInt = geneToInt(&genome.genes[idx]);
  geneInt = geneInt ^ (1 << (rand() % 32));
  genome.genes[idx] = intToGene(geneInt);
  return genome;
}

Organism makeOffspring(Organism *a, Organism *b, int worldWidth,
                       int worldHeight, Organism *otherOrgs,
                       int otherOrgsCount) {
  Organism org = {
      .pos =
          (Pos){
              .x = rand() % worldWidth,
              .y = rand() % worldHeight,
          },
      .genome = mutate(reproduce(&a->genome, &b->genome), SIM_MUTATION_RATE),
      .alive = true,
      .didCollide = false,
      .energyLevel = 1.0,
      .direction = getRandomDirection()};

  while (getOrganismByPos(org.pos, otherOrgs, otherOrgsCount, false) ||
         isPosInObstacle(org.pos)) {
    org.pos.x = rand() % worldWidth;
    org.pos.y = rand() % worldWidth;
  }

  organismBuildNeuralNet(&org);

  return org;
}

void destroyOrganism(Organism *org) {
  organismDestroyNeuralNet(org);
  free(org->genome.genes);
  org->genome.genes = NULL;
  org->genome.count = 0;
}

void dumpOrganismNet(Organism *org) {
  printf("BREAKDOWN OF NET:\n");
  for (int i = 0; i < org->net.connectionCount; i++) {
    NeuralConnection *connection = &org->net.connections[i];

    printf("Connection %i:\n", i);
    printf("  Weight = %.2f\n", connection->weight);
    printf("  Source = #%d\n", connection->sourceId);
    printf("  Sink   = #%d\n", connection->sinkId);
  }

  for (int i = 0; i < org->net.neuronCount; i++) {
    Neuron *neuron = &org->net.neurons[i];

    printf("Neuron %d:\n", i);
    printf("  ID      = %d\n", neuron->id);
    printf("  Type    = %s\n",
           (neuron->type == NEURON_INPUT
                ? "INPUT"
                : (neuron->type == NEURON_OUTPUT ? "OUTPUT" : "INTERNAL")));

    if (neuron->type == NEURON_INPUT)
      printf("  Type 2  = %s\n", InputTypeStrings[neuron->id & 0x7f]);
    if (neuron->type == NEURON_OUTPUT)
      printf("  Type 2  = %s\n", OutputTypeStrings[neuron->id & 0x7f]);

    printf("  Inputs  = %d\n", neuron->inputs);
    printf("  Outputs = %d\n", neuron->outputs);
  }
  printf("\n");
}

bool bottomSelector(Organism *org) {
  return (org->pos.x < (SIM_WIDTH / 2)) && (org->pos.y < (SIM_HEIGHT / 2));
}

bool centerXSelector(Organism *org) {
  return ((org->pos.x > (SIM_WIDTH / 3)) && (org->pos.x < (2 * SIM_WIDTH / 3)));
}

bool centerYSelector(Organism *org) {
  return ((org->pos.y > (SIM_HEIGHT / 3)) &&
          (org->pos.y < (2 * SIM_HEIGHT / 3)));
}

bool collidedSelector(Organism *org) { return org->didCollide; }

bool hasEnoughEnergy(Organism *org) { return org->energyLevel > 0.7f; }

bool centerSelector(Organism *org) {
  return centerXSelector(org) && centerYSelector(org);
}

bool circleCenterSelector(Organism* org) {
  return
    ((SIM_WIDTH / 2 - org->pos.x) * (SIM_WIDTH / 2 - org->pos.x) +
    (SIM_HEIGHT / 2 - org->pos.y) * (SIM_HEIGHT / 2 - org->pos.y)) < (32 * 32);
}

bool triangleSelector(Organism *org) { return (org->pos.x >= org->pos.y); }

bool leftSelector(Organism *org) { return org->pos.x < SIM_WIDTH * 0.2; }
bool rightSelector(Organism *org) { return org->pos.x > SIM_WIDTH * 0.8; }

bool leftAndRightSelector(Organism *org) {
  return leftSelector(org) || rightSelector(org);
}

void findMates(Organism orgs[], int population, Organism **outA,
               Organism **outB) {
  // finds two distinct organisms that are alive
  *outA = NULL;
  *outB = NULL;

  if (population == 0) {
    fprintf(stderr, "Cannot call findMates with a population of 0!\n");
    exit(1);
    return;
  }

  while (*outA == NULL) {
    *outA = &orgs[rand() % population];
    if (!(*outA)->alive) {
      *outA = NULL;
    }
  }

  while (*outB == NULL && *outA != *outB) {
    *outB = &orgs[rand() % population];
    if (!(*outB)->alive) {
      *outB = NULL;
    }
  }
}

Genome reproduce(Genome *a, Genome *b) {
  int largerCount = a->count > b->count ? a->count : b->count;

  Genome genome = {.count = largerCount,
                   .genes = calloc(largerCount, sizeof(Gene))};

  for (int i = 0; i < largerCount; i++) {
    if (i < a->count && i < b->count) {
      if (rand() % 2 == 0) {
        genome.genes[i] = a->genes[i];
      } else {
        genome.genes[i] = b->genes[i];
      }
    } else if (i < a->count) {
      genome.genes[i] = a->genes[i];
    } else {
      genome.genes[i] = b->genes[i];
    }
  }

  return genome;
}

static volatile bool interrupted = false;

void signalHandler(int sig) { interrupted = true; }

int main(int argc, char *argv[]) {
  visInit(SIM_WIDTH, SIM_HEIGHT);
  signal(SIGINT, &signalHandler);

  int seed;

  if (argc == 2) 
  {
    if (sscanf(argv[1], "%d", &seed) != 1) {
      fprintf(stderr, "Could not parse seed from argument.\n");
      seed = time(NULL);
    }
  } else {
    seed = time(NULL);
  }

  srand(seed);
  printf("Seed is %d\n", seed);

  Organism *orgs = calloc(SIM_POPULATION, sizeof(Organism));
  Organism *nextGenOrgs = calloc(SIM_POPULATION, sizeof(Organism));

  visSetObstacles(obstacles, obstaclesCount);

  for (int i = 0; i < SIM_POPULATION; i++) {
    orgs[i] = makeRandomOrganism(SIM_NUM_GENES, SIM_WIDTH, SIM_HEIGHT, orgs, i);
  }

  float Ao10Buffer[10] = {0.0f};
  int Ao10Idx = 0;

  for (int g = 0; g < SIM_MAX_GENERATIONS; g++) {
    visSetGeneration(g);

    for (int step = 0; step < SIM_GEN_STEPS; step++) {

      visSetStep(step);
      visDrawStep(orgs, SIM_POPULATION, false);

      for (int i = 0; i < SIM_POPULATION; i++) {
        organismRunStep(&orgs[i], orgs, i, step, SIM_GEN_STEPS);
      }

      if (interrupted || visGetWantsToQuit())
        break;
    }

    if (interrupted || visGetWantsToQuit()) {
      break;
    }

    // assert that we don't have two organisms occupying the same cell
    int occupiedCells = 0;
    for (int i = 0; i < SIM_POPULATION; i++) {
      if (!orgs[i].alive)
        continue;
      // printf("Org #%d has pos (%d, %d) at end of gen %d\n", i, orgs[i].pos.x,
      //        orgs[i].pos.y, g);
      if (getOrganismByPos(orgs[i].pos, orgs, i, true)) {
        occupiedCells++;
      }
    }
    if (occupiedCells) {
      printf("Found %d occupied cells\n", occupiedCells);
    }

    int survivors = 0;
    int deadBeforeSelection = 0;
    int deadAfterSelection = 0;
    for (int i = 0; i < SIM_POPULATION; i++) {
      Organism *org = &orgs[i];

      if (!org->alive) {
        deadBeforeSelection++;
        continue;
      }

      if (circleCenterSelector(org) /* && hasEnoughEnergy(org)*/) {
        survivors++;
      } else {
        deadAfterSelection++;
        org->alive = false;
      }
    }

    visDrawStep(orgs, SIM_POPULATION, true);

    float survivalRate = (float)survivors * 100.0f / SIM_POPULATION;

    Ao10Buffer[Ao10Idx] = survivalRate;
    Ao10Idx = (Ao10Idx + 1) % 10;

    float Ao10 = 0.0f;
    for (int a = 0; a < 10; a++) {
      Ao10 += Ao10Buffer[a];
    }
    Ao10 /= (float)(g < 10 ? g + 1 : 10);

    printf("Gen %d survival rate is %d/%d (%03.2f%%, %03.2f%% Ao10) with %d "
           "dead before and "
           "%d dead after selection.\n",
           g, survivors, SIM_POPULATION, survivalRate, Ao10,
           deadBeforeSelection, deadAfterSelection);

    if (survivors <= 1) {
      break;
    }

    for (int i = 0; i < SIM_POPULATION; i++) {
      Organism *a, *b;
      findMates(orgs, SIM_POPULATION, &a, &b);
      nextGenOrgs[i] = makeOffspring(a, b, SIM_WIDTH, SIM_HEIGHT, orgs, i);
    }

    for (int i = 0; i < SIM_POPULATION; i++) {
      destroyOrganism(&orgs[i]);
    }

    Organism* tmp = orgs;
    orgs = nextGenOrgs;
    nextGenOrgs = tmp;

    if (interrupted || survivors <= 1 || visGetWantsToQuit())
      break;
  }

  for (int i = 0; i < SIM_POPULATION; i++) {
    destroyOrganism(&orgs[i]);
  }

  visDestroy();

  free(orgs);
  free(nextGenOrgs);

  return EXIT_SUCCESS;
}
