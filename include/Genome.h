#ifndef Genome_h
#define Genome_h

#include "Common.h"
#include "Arena.h"

Genome copyGenome(Arena* arena, Genome* src);
Genome makeRandomGenome(Arena* arena, uint8_t numGenes);
Genome mutateGenome(Genome genome, float mutationRate, bool* didMutate);
Genome reproduce(Arena* arena, Genome *a, Genome *b);

#endif
