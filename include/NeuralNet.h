#ifndef NeuralNet_h
#define NeuralNet_h

#include "Common.h"
#include "Arena.h"

NeuralNet buildNeuralNet(Arena* arena, Genome *genome, Simulation* sim);
void destroyNeuralNet(NeuralNet *net);
Neuron *findNeuronById(Neuron* neurons, size_t neuronCount, uint16_t id);
NeuralNet copyNeuralNet(Arena* arena, NeuralNet* src);

#endif
