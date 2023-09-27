#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define IN_BASE 0x8000
#define OUT_BASE 0x4000
#define INTERNAL_BASE 0x200

#define INTERNAL_MAX 1

typedef enum
{
  IN_WORLD_X,
  IN_WORLD_Y,
  IN_MAX
} InputType;

char *InputTypeStrings[IN_MAX] = {
    "WORLD_X",
    "WORLD_Y"};

typedef enum
{
  OUT_MOVE_X,
  OUT_MOVE_Y,
  OUT_MOVE_RANDOM,
  OUT_MAX
} OutputType;

char *OutputTypeStrings[OUT_MAX] = {
    "MOVE_X",
    "MOVE_Y",
    "MOVE_RANDOM"};

typedef struct
{
  uint16_t x;
  uint16_t y;
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

Gene intToGene(uint32_t n)
{
  return (Gene){
      .sourceIsInput = (bool)((n >> 31) & 0x01),
      .sourceId = (uint8_t)((n >> 24) & 0x7f),
      .sinkIsOutput = (bool)((n >> 23) & 0x01),
      .sinkId = (uint8_t)((n >> 16) & 0x7f),
      .weight = (uint16_t)(n & 0xffff),
  };
}

uint32_t geneToInt(Gene *gene)
{
  return (uint32_t)(((gene->sourceIsInput << 7) | gene->sourceId) << 24) |
         (uint32_t)(((gene->sinkIsOutput << 7) | gene->sinkId) << 16) |
         (uint32_t)gene->weight;
}

void testGeneCreation()
{
  uint32_t geneInt = rand() % UINT32_MAX;

  Gene gene = intToGene(geneInt);

  uint32_t newGeneInt = geneToInt(&gene);

  if (geneInt != newGeneInt)
  {
    fprintf(stderr, "ERROR: %08X /= %08X\n", geneInt, newGeneInt);
    exit(1);
  }
}

char *geneToString(Gene *gene)
{
  char *buffer = calloc(9, sizeof(char));
  snprintf(buffer, 9, "%08X", geneToInt(gene));
  return buffer;
}

char *genomeToString(Genome *genome)
{
  size_t size = 9 * genome->count + 1;
  char *buffer = calloc(size, sizeof(char));
  for (int i = 0; i < genome->count; i++)
  {
    char *geneBuffer = geneToString(&genome->genes[i]);
    sprintf(&buffer[i * 9], "%s", geneBuffer);
    if (i + 1 != genome->count)
    {
      buffer[i * 9 + 8] = ' ';
    }
    free(geneBuffer);
  }
  return buffer;
}

Neuron *findNeuronById(Neuron *neurons, size_t n, uint16_t id)
{
  for (int i = 0; i < n; i++)
  {
    if (neurons[i].id == id)
    {
      return &neurons[i];
    }
  }
  return NULL;
}

void organismBuildNeuralNet(Organism *org)
{
  Neuron neurons[128] = {0};
  uint8_t usedNeurons = 0;

  NeuralConnection connections[128] = {0};
  uint8_t usedConnections = 0;

  for (int i = 0; i < org->genome.count; i++)
  {
    Gene *gene = &org->genome.genes[i];

    uint16_t sourceId = gene->sourceId;
    if (gene->sourceIsInput)
    {
      sourceId %= IN_MAX;
      sourceId |= IN_BASE;
    }
    else
    {
      sourceId %= INTERNAL_MAX;
      sourceId |= INTERNAL_BASE;
    }

    uint16_t sinkId = gene->sinkId;
    if (gene->sinkIsOutput)
    {
      sinkId %= OUT_MAX;
      sinkId |= OUT_BASE;
    }
    else
    {
      sinkId %= INTERNAL_MAX;
      sinkId |= INTERNAL_BASE;
    }

    Neuron *source = findNeuronById(neurons, usedNeurons, sourceId);
    if (source == NULL)
    {
      source = &neurons[usedNeurons++];
      source->id = sourceId;
      source->type = gene->sourceIsInput ? NEURON_INPUT : NEURON_INTERNAL;
      source->state = 0.0f;
      source->inputs = 0;
      source->outputs = 1;
      source->inputsVisited = 0;
      source->outputsVisited = 0;
    }
    else
    {
      source->outputs++;
    }

    Neuron *sink = findNeuronById(neurons, usedNeurons, sinkId);
    if (sink == NULL)
    {
      sink = &neurons[usedNeurons++];
      sink->id = sinkId;
      sink->type = gene->sinkIsOutput ? NEURON_OUTPUT : NEURON_INTERNAL;
      sink->state = 0.0f;
      sink->inputs = 1;
      sink->outputs = 0;
      sink->inputsVisited = 0;
      sink->outputsVisited = 0;
    }
    else
    {
      sink->inputs++;
    }

    // printf("Source for neuron %d has id %d\n", i, source->id);
    // printf("Sink for neuron %d has id %d\n", i, sink->id);

    NeuralConnection *conn = &connections[usedConnections++];
    conn->sourceId = source->id;
    conn->sinkId = sink->id;
    conn->weight = (float)gene->weight * 8.0f / 65536.0f - 4.0f; // between -4.0 and +4.0
    conn->visited = false;
  }

  org->net.connectionCount = usedConnections;
  org->net.connections = calloc(usedConnections, sizeof(NeuralConnection));
  memcpy(org->net.connections, connections, usedConnections * sizeof(NeuralConnection));

  org->net.neuronCount = usedNeurons;
  org->net.neurons = calloc(usedNeurons, sizeof(Neuron));
  memcpy(org->net.neurons, neurons, usedNeurons * sizeof(Neuron));

  // Neuron *source = findNeuronById(org->net.neurons, org->net.neuronCount, 32768);
  // printf("Neuron 0 has %d inputs and %d outputs\n", source->inputs, source->outputs);
  // printf("Made net with %d connections between %d neurons\n", org->net.connectionCount, org->net.neuronCount);

  // Neuron *sink = findNeuronById(org->net.neurons, org->net.neuronCount, 16384);
  // printf("Net sink is %p, source is %p\n", sink, source);
}

void organismDestroyNeuralNet(Organism *org)
{
  free(org->net.connections);
  free(org->net.neurons);
  org->net.connections = NULL;
  org->net.neurons = NULL;
  org->net.connectionCount = 0;
  org->net.neuronCount = 0;
}

void organismRunStep(Organism *org)
{
  // reset
  for (int i = 0; i < org->net.connectionCount; i++)
  {
    org->net.connections[i].visited = false;
  }
  for (int i = 0; i < org->net.neuronCount; i++)
  {
    org->net.neurons[i].inputsVisited = 0;
    org->net.neurons[i].outputsVisited = 0;
    org->net.neurons[i].prevState = org->net.neurons[i].state;
    org->net.neurons[i].state = 0.0f;
  }

  // excite
  for (int i = 0; i < org->net.neuronCount; i++)
  {
    Neuron *input = &org->net.neurons[i];
    if (input->type != NEURON_INPUT)
    {
      continue;
    }

    switch (input->id & 0xff)
    {
    case IN_WORLD_X:
      input->state = (float)org->pos.x / 128.0f;
      break;
    case IN_WORLD_Y:
      input->state = (float)org->pos.y / 128.0f;
      break;
    }

    // printf("Excited neuron #%d to level %.2f\n", input->id, input->state);
  }

  // compute
  int visits;
  do
  {
    visits = 0;

    for (int i = 0; i < org->net.connectionCount; i++)
    {
      NeuralConnection *connection = &org->net.connections[i];

      Neuron *source = findNeuronById(org->net.neurons, org->net.neuronCount, connection->sourceId);
      Neuron *sink = findNeuronById(org->net.neurons, org->net.neuronCount, connection->sinkId);

      // if the source has inputs and they haven't been fulfilled then don't do anything here yet
      if (source->inputs && (source->inputsVisited < source->inputs))
      {
        continue;
      }

      // if the connection has already been visited then skip
      if (connection->visited)
      {
        continue;
      }

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
      connection->visited = true;
      // printf("Visited connection between neuron #%d and neuron #%d\n", source->id, sink->id);

      // normalise the sink state if it has been completed between -1.0 and 1.0.
      if (sink->inputsVisited == sink->inputs)
      {
        sink->state = tanhf(sink->state / (float)sink->inputs);
        // printf("Normalised neuron #%d state to %.2f\n", sink->id, sink->state);
      }

      visits++;
      // printf("Source has %d of %d inputs fulfilled\n", source->inputsVisited, source->inputs);
    }
  } while (visits > 0);

  // act
  for (int i = 0; i < org->net.neuronCount; i++)
  {
    Neuron *output = &org->net.neurons[i];

    if (output->type != NEURON_OUTPUT)
    {
      // printf("Neuron #%d is not an output\n", output->id);
      continue;
    }

    // printf("Output neuron #%d has level %.2f\n", output->id, output->state);

    switch (output->id & 0xff)
    {
    case OUT_MOVE_X:
      if (output->state >= 0.5f)
      {
        org->pos.x++;
      }
      else if (output->state <= -0.5f)
      {
        org->pos.x--;
      }
      break;
    case OUT_MOVE_Y:
      if (output->state >= 0.5f)
      {
        org->pos.y++;
      }
      else if (output->state <= -0.5f)
      {
        org->pos.y--;
      }
      break;
    case OUT_MOVE_RANDOM:
      if (fabs(output->state) >= 0.5f)
      {
        org->pos.x += rand() % 3 - 1;
        org->pos.y += rand() % 3 - 1;
      }
      break;
    }
  }

  // enforce collisions with edge
  if (org->pos.x >= 128)
  {
    org->pos.x = 127;
  }
  else if (org->pos.x < 0)
  {
    org->pos.x = 0;
  }
  if (org->pos.y >= 128)
  {
    org->pos.y = 127;
  }
  else if (org->pos.y < 0)
  {
    org->pos.y = 0;
  }
}

Genome makeRandomGenome(uint8_t numGenes)
{
  Genome genome = {
      .count = numGenes,
      .genes = calloc(numGenes, sizeof(Gene))};

  for (int i = 0; i < numGenes; i++)
  {
    genome.genes[i] = intToGene(rand() % UINT32_MAX);
    if (genome.genes[i].sourceIsInput)
    {
      printf("CREATED GENE WITH INPUT!\n");
    }
  }

  return genome;
}

Organism makeRandomOrganism(uint8_t numGenes, int worldWidth, int worldHeight)
{
  Organism org = {
      .pos = (Pos){
          .x = rand() % worldWidth,
          .y = rand() % worldHeight},
      .genome = makeRandomGenome(numGenes),
  };

  organismBuildNeuralNet(&org);

  return org;
}

void destroyOrganism(Organism *org)
{
  organismDestroyNeuralNet(org);
  free(org->genome.genes);
  org->genome.genes = NULL;
  org->genome.count = 0;
}

void dumpOrganismNet(Organism *org)
{
  printf("BREAKDOWN OF NET:\n");
  for (int i = 0; i < org->net.connectionCount; i++)
  {
    NeuralConnection *connection = &org->net.connections[i];

    printf("Connection %i:\n", i);
    printf("  Weight = %.2f\n", connection->weight);
    printf("  Source = #%d\n", connection->sourceId);
    printf("  Sink   = #%d\n", connection->sinkId);
  }

  for (int i = 0; i < org->net.neuronCount; i++)
  {
    Neuron *neuron = &org->net.neurons[i];

    printf("Neuron %d:\n", i);
    printf("  ID      = %d\n", neuron->id);
    printf("  Type    = %s\n", (neuron->type == NEURON_INPUT ? "INPUT" : (neuron->type == NEURON_OUTPUT ? "OUTPUT" : "INTERNAL")));

    if (neuron->type == NEURON_INPUT)
      printf("  Type 2  = %s\n", InputTypeStrings[neuron->id & 0x7f]);
    if (neuron->type == NEURON_OUTPUT)
      printf("  Type 2  = %s\n", OutputTypeStrings[neuron->id & 0x7f]);

    printf("  Inputs  = %d\n", neuron->inputs);
    printf("  Outputs = %d\n", neuron->outputs);
  }
  printf("\n");
}

int main(int argc, char *argv[])
{
  srand(time(NULL));

  // Gene gene = {
  //     .sourceIsInput = true,
  //     .sourceId = IN_WORLD_X,
  //     .sinkIsOutput = true,
  //     .sinkId = OUT_MOVE_X,
  //     .weight = rand() % 65536,
  // };

  // printf("Weight is %d\n", gene.weight);

  // Organism org = {
  //     .pos = (Pos){
  //         .x = (uint16_t)(rand() % 128),
  //         .y = (uint16_t)(rand() % 128)},
  //     .genome = (Genome){.count = 1, .genes = &gene},
  // };

  // organismBuildNeuralNet(&org);

  // char *geneString = genomeToString(&org.genome);
  // printf("Organism is at (%d,%d)\n", org.pos.x, org.pos.y);
  // printf("Organism has genome %s\n", geneString);
  // free(geneString);

  testGeneCreation();

  uint64_t n = 0;
  while (true)
  {
    n++;
    Organism org = makeRandomOrganism(2, 128, 128);
    dumpOrganismNet(&org);
    return 0;
    Pos start = org.pos;

    for (int i = 0; i < 10; i++)
    {
      // printf("\n");
      organismRunStep(&org);
      // printf("Organism is at (%d,%d)\n", org.pos.x, org.pos.y);
    }

    if (start.x != org.pos.x || start.y != org.pos.y)
    {
      printf("\nTook %ld attempts.\n", n);
      dumpOrganismNet(&org);

      char *genomeString = genomeToString(&org.genome);
      printf("Genome: %s\n", genomeString);
      free(genomeString);

      printf("Organism moved!\n");

      break;
    }

    if (n % 100000 == 0)
    {
      printf(".");
      fflush(stdout);
    }

    destroyOrganism(&org);
  }

  return EXIT_SUCCESS;
}
