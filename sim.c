#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define IN_BASE (1 << 15)
#define OUT_BASE (1 << 14)
#define INTERNAL_BASE (1 << 13)

typedef enum {
  IN_WORLD_X,
  IN_WORLD_Y,
  IN_MAX
} InputType;

typedef enum {
  OUT_MOVE_X,
  OUT_MOVE_Y,
  OUT_MOVE_RANDOM,
  OUT_MAX
} OutputType;

typedef struct {
  uint16_t x;
  uint16_t y;
} Pos;

typedef struct {
  uint16_t w;
  uint16_t h;
} Size;

typedef struct {
  bool sourceIsInput : 1;
  uint8_t sourceId : 7;
  bool sinkIsOutput : 1;
  uint8_t sinkId : 7;
  uint16_t weight;
} Gene;

typedef struct {
  uint8_t count;
  Gene* genes;
} Genome;

struct Neuron_t;

typedef struct {
  struct Neuron_t* source;
  struct Neuron_t* sink;
  float weight;
  bool visited;
} NeuralConnection;

typedef struct Neuron_t {
  uint16_t id;
  float prevState;
  float state;
  uint8_t inputs;
  uint8_t inputsVisited;
  uint8_t outputs;
  uint8_t outputsVisited;
} Neuron;

typedef struct {
  uint16_t neuronCount;
  Neuron* neurons;
  uint16_t connectionCount;
  NeuralConnection* connections;
} NeuralNet;

typedef struct {
  Pos pos;
  Genome genome;
  NeuralNet net;
} Organism;

typedef struct {
  uint16_t step;
  Organism* organisms;
} Generation;

typedef struct {
  Size size;
} Simulation;

uint32_t geneToInt(Gene* gene)
{
  return
    (uint32_t)(((gene->sourceIsInput << 7) | gene->sourceId) << 24) |
    (uint32_t)(((gene->sinkIsOutput  << 7) | gene->sinkId  ) << 16) |
    (uint32_t)gene->weight;
}

char* geneToString(Gene* gene)
{ 
  char* buffer = calloc(9, sizeof(char));
  snprintf(buffer, 9, "%08X", geneToInt(gene));
  return buffer;
}

char* genomeToString(Genome* genome)
{
  size_t size = 9 * genome->count + 1;
  char* buffer = calloc(size, sizeof(char));
  for (int i = 0; i < genome->count; i++)
  {
    char* geneBuffer = geneToString(&genome->genes[i]);
    sprintf(&buffer[i * 9], "%s", geneBuffer);
    if (i + 1 != genome->count) {
      buffer[i * 9 + 8] = ' ';
    }
    free(geneBuffer);
  }
  return buffer;
}

Neuron* findNeuronById(Neuron* neurons, size_t n, uint16_t id)
{
  for (int i = 0; i < n; i++)
  {
    if (neurons[i].id == id) {
      return &neurons[i];
    }
  }
  return NULL;
}

void organismBuildNeuralNet(Organism* org)
{
  Neuron neurons[128] = { 0 };
  uint8_t usedNeurons = 0;
  
  NeuralConnection connections[128] = { 0 };
  uint8_t usedConnections = 0;

  for (int i = 0; i < org->genome.count; i++)
  {
    Gene* gene = &org->genome.genes[i];

    uint16_t sourceId = gene->sourceId;
    if (gene->sourceIsInput) {
      sourceId |= IN_BASE;
    } else {
      sourceId |= INTERNAL_BASE;
    }

    uint16_t sinkId = gene->sinkId;
    if (gene->sinkIsOutput) {
      sinkId |= OUT_BASE;
    } else {
      sinkId |= INTERNAL_BASE;
    }

    Neuron* source = findNeuronById(neurons, usedNeurons, sourceId);
    if (source == NULL) {
      source = &neurons[usedNeurons++];
      source->id = sourceId;
      source->state = 0.0f;
      source->inputs = 0;
      source->outputs = 1;
      source->inputsVisited = 0;
      source->outputsVisited = 0;
    } else {
      source->outputs++;
    }

    Neuron* sink = findNeuronById(neurons, usedNeurons, sinkId);
    if (sink == NULL) {
      sink = &neurons[usedNeurons++];
      sink->id = sinkId;
      sink->state = 0.0f;
      sink->inputs = 1;
      sink->outputs = 0;
      sink->inputsVisited = 0;
      sink->outputsVisited = 0;
    } else {
      sink->inputs++;
    }

    NeuralConnection* conn = &connections[usedConnections++];
    conn->source = source;
    conn->sink = sink;
    conn->weight = (float)gene->weight * 8.0f / 65536.0f - 4.0f; // between -4.0 and +4.0
    conn->visited = false;
  }

  org->net.connectionCount = usedConnections;
  org->net.connections = calloc(usedConnections, sizeof(NeuralConnection));
  memcpy(org->net.connections, connections, usedConnections * sizeof(NeuralConnection));

  org->net.neuronCount = usedNeurons;
  org->net.neurons = calloc(usedNeurons, sizeof(Neuron));
  memcpy(org->net.neurons, neurons, usedNeurons * sizeof(Neuron));
}

void organismRunStep(Organism* org)
{
  // reset
  for (int i = 0; i < org->net.connectionCount; i++) {
    org->net.connections[i].visited = false;
  }
  for (int i = 0; i < org->net.neuronCount; i++) {
    org->net.neurons[i].inputsVisited = 0;
    org->net.neurons[i].outputsVisited = 0;
    org->net.neurons[i].prevState = org->net.neurons[i].state;
    org->net.neurons[i].state = 0.0f;
  }

  // play
  int visits;
  do {
    visits = 0;

    for (int i = 0; i < org->net.connectionCount; i++)
    {
      NeuralConnection* connection = &org->net.connections[i];

      Neuron* source = connection->source;
      Neuron* sink = connection->sink;

      // if the source hasn't had all its inputs fulfilled then don't do anything here yet
      if ((source->inputsVisited < source->inputs) && (source->inputs > 0)) { continue; }

      // if the connection has already been visited then skip
      if (connection->visited) { continue; }

      if (sink == source)
      {
        sink->state += connection->weight * source->prevState;
      }
      else
      {
        sink->state += connection->weight * source->state;
      }

      sink->inputsVisited++;
      source->outputsVisited++;

      // normalise the sink state if it has been completed.
      if (sink->inputsVisited == sink->inputs)
      {
        sink->state = tanhf(sink->state / (float)sink->inputs);
      }

      visits++;
      printf("Source has %d of %d inputs fulfilled\n", source->inputsVisited, source->inputs);
    }
  } while (visits > 0);
}

int main(int argc, char* argv[])
{
  time_t t = time(NULL);
  srandom(t);

  Gene gene = {
    .sourceIsInput = true,
    .sourceId = IN_WORLD_X,
    .sinkIsOutput = true,
    .sinkId = OUT_MOVE_X,
    .weight = random() % 65536,
  };

  printf("Weight is %d\n", gene.weight);
  
  Organism org = {
    .pos = (Pos){
      .x = (uint16_t)(random() % 128),
      .y = (uint16_t)(random() % 128)
    },
    .genome = (Genome){
      .count = 1,
      .genes = &gene
    },
  };

  organismBuildNeuralNet(&org);

  char* geneString = genomeToString(&org.genome);
  printf("Organism is at (%d,%d)\n", org.pos.x, org.pos.y);
  printf("Organism has genome %s\n", geneString);
  free(geneString);

  organismRunStep(&org);

  
  return EXIT_SUCCESS;
}
