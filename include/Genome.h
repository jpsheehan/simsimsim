#ifndef Genome_h
#define Genome_h

#include "Common.h"

Genome copyGenome(Genome* src, Gene* geneBuffer);
Genome makeRandomGenome(uint8_t numGenes, Gene* geneBuffer);
Genome mutateGenome(Genome genome, float mutationRate, bool* didMutate);
Genome reproduce(Genome *a, Genome *b, Gene* geneBuffer);

#endif
