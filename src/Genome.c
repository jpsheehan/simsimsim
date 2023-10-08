#include "Genome.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// make a deep copy of the genome
Genome copyGenome(Genome* src)
{
    Genome dest = *src;

    dest.genes = calloc(src->count, sizeof(Gene));
    memcpy(dest.genes, src->genes, sizeof(Gene) * src->count);

    return dest;
}


Gene intToGene(uint32_t n)
{
    return (Gene) {
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

    if (geneInt != newGeneInt) {
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
    for (int i = 0; i < genome->count; i++) {
        char *geneBuffer = geneToString(&genome->genes[i]);
        sprintf(&buffer[i * 9], "%s", geneBuffer);
        if (i + 1 != genome->count) {
            buffer[i * 9 + 8] = ' ';
        }
        free(geneBuffer);
    }
    return buffer;
}

Genome mutateGenome(Genome genome, float mutationRate)
{
    if ((float)rand() / (float)RAND_MAX >= mutationRate) {
        return genome;
    }

    // flip a random bit
    int idx = rand() % genome.count;
    int geneInt = geneToInt(&genome.genes[idx]);
    geneInt = geneInt ^ (1 << (rand() % 32));
    genome.genes[idx] = intToGene(geneInt);
    return genome;
}

uint32_t rand_uint32(void)
{
    return (uint32_t)((uint16_t)(rand()) << 16) | (uint16_t)rand();
}

Genome makeRandomGenome(uint8_t numGenes)
{
    Genome genome = {.count = numGenes, .genes = calloc(numGenes, sizeof(Gene))};

    for (int i = 0; i < numGenes; i++) {
        genome.genes[i] = intToGene(rand_uint32());
    }

    return genome;
}