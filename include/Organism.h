#ifndef Organism_h
#define Organism_h

#include "Common.h"

Organism makeRandomOrganism(Simulation* sim, Organism **organismsByPosition);
Organism *getOrganismByPos(Pos pos, Simulation* sim, Organism **orgsByPosition,
                           bool aliveOnly);
void destroyOrganism(Organism *org);
Organism makeOffspring(Organism *a, Organism *b, Simulation* sim, Organism **orgsByPosition);
void findMates(Organism orgs[], int population, Organism **outA,
               Organism **outB);
void organismRunStep(Organism *org, Organism **organismsByPosition, Simulation* sim,
                     int currentStep, int maxStep);
#endif
