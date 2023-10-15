#ifndef NeuralNet_h
#define NeuralNet_h

#include "Common.h"

NeuralNet buildNeuralNet(Genome *genome, Simulation* sim, Neuron* neuronBuffer, NeuralConnection* connectionBuffer);
void destroyNeuralNet(NeuralNet *net);
Neuron *findNeuronById(Neuron* neurons, size_t neuronCount, uint16_t id);
NeuralNet copyNeuralNet(NeuralNet* src, Neuron* neuronBuffer, NeuralConnection* connectionBuffer);

#endif
