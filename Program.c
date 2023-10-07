#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Simulator.h"
#include "Selectors.h"

int main(int argc, const char* argv[])
{
  Simulation sim;

  if (argc == 2)
  {
    if (sscanf(argv[1], "%d", &sim.seed) != 1) {
      fprintf(stderr, "Could not parse seed from argument.\n");
      sim.seed = time(NULL);
    }
  } else {
    sim.seed = time(NULL);
  }

  sim.selector = &leftSelector;
  sim.mutationRate = 0.01;
  sim.obstacles = NULL;
  sim.obstaclesCount = 0;
  sim.size = (Size){ .w = 128, .h = 128 };
  sim.energyToMove = 0.01;
  sim.energyToRest = 0.01;
  sim.maxInternalNeurons = 2;
  sim.population = 1000;
  sim.stepsPerGeneration = 150;
  sim.numberOfGenes = 12;
  sim.maxGenerations = 100;

  runSimulation(&sim);

  return EXIT_SUCCESS;
}