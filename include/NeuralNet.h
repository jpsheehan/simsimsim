#ifndef NeuralNet_h
#define NeuralNet_h

#include "Common.h"

NeuralNet buildNeuralNet(Genome *genome, Simulation* sim);
void destroyNeuralNet(NeuralNet *net);
Neuron *findNeuronById(Neuron* neurons, size_t neuronCount, uint16_t id);
NeuralNet copyNeuralNet(NeuralNet* src);

#endif
