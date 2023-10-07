#ifndef Organism_h
#define Organism_h

#include "Common.h"

Organism makeRandomOrganism(uint8_t numGenes, Simulation* sim,
                            Organism *otherOrgs, int otherOrgsCount);
Organism *getOrganismByPos(Pos pos, Organism *orgs, int orgsCount,
                           bool aliveOnly);
                           void destroyOrganism(Organism *org);
                           Organism makeOffspring(Organism *a, Organism *b, Simulation* sim, Organism *otherOrgs,
                       int otherOrgsCount);
                       void findMates(Organism orgs[], int population, Organism **outA,
               Organism **outB);
               void organismRunStep(Organism *org, Organism *otherOrgs, Simulation* sim, int otherOrgsCount,
                     int currentStep, int maxStep);
#endif
