#ifndef Organism_h
#define Organism_h

#include "Common.h"

Organism makeRandomOrganism(Simulation* sim, Organism **organismsByPosition, Neuron* neuronBuffer, NeuralConnection* connectionBuffer, Gene* geneBuffer);
Organism *getOrganismByPos(Pos pos, Simulation* sim, Organism **orgsByPosition,
                           bool aliveOnly);
void destroyOrganism(Organism *org);
Organism makeOffspring(Organism *a, Organism *b, Simulation* sim, Organism **orgsByPosition, Neuron* neuronBuffer, NeuralConnection* connectionBuffer, Gene* geneBuffer);
void findMates(Organism orgs[], int population, Organism **outA,
               Organism **outB);
void organismRunStep(Organism *org, Organism **organismsByPosition, Organism** prevOrgsByPosition, Simulation* sim,
                     int currentStep);

Organism copyOrganism(Organism *src, Neuron* neuronBuffer, NeuralConnection* connectionBuffer, Gene* geneBuffer);
void copyOrganismMutableState(Organism* dest, Organism* src);

#endif
