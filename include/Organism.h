#ifndef Organism_h
#define Organism_h

#include "Common.h"
#include "Arena.h"

Organism makeRandomOrganism(Arena* arena, Simulation* sim, Organism **organismsByPosition);
Organism *getOrganismByPos(Pos pos, Simulation* sim, Organism **orgsByPosition,
                           bool aliveOnly);
void destroyOrganism(Organism *org);
Organism makeOffspring(Arena* arena, Organism *a, Organism *b, Simulation* sim, Organism **orgsByPosition);
void findMates(Organism orgs[], int population, Organism **outA,
               Organism **outB);
void organismRunStep(Organism *org, Organism **organismsByPosition, Organism** prevOrgsByPosition, Simulation* sim,
                     int currentStep);

Organism copyOrganism(Arena* arena, Organism *src);
void copyOrganismMutableState(Organism* dest, Organism* src);

#endif
