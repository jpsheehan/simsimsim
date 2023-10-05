#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Simulator.h"

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

  runSimulation(sim);

  return EXIT_SUCCESS;
}