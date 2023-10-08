#ifndef Genome_h
#define Genome_h

#include "Common.h"

Genome copyGenome(Genome* src);
Genome makeRandomGenome(uint8_t numGenes);
Genome mutateGenome(Genome genome, float mutationRate);

#endif
