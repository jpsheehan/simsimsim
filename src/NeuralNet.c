#include "NeuralNet.h"

#include <stdlib.h>
#include <string.h>

Neuron *findNeuronById(Neuron* neurons, size_t neuronCount, uint16_t id)
{
    for (int i = 0; i < neuronCount; i++) {
        if (neurons[i].id == id) {
            return &neurons[i];
        }
    }
    return NULL;
}

NeuralNet buildNeuralNet(Genome *genome, Simulation* sim, Neuron* neuronBuffer, NeuralConnection* connectionBuffer)
{
    uint8_t usedNeurons = 0;
    uint8_t usedConnections = 0;

    for (int i = 0; i < genome->count; i++) {
        Gene *gene = &genome->genes[i];

        uint16_t sourceId = gene->sourceId;
        if (gene->sourceIsInput) {
            sourceId %= IN_MAX;
            sourceId |= IN_BASE;
        } else {
            sourceId %= sim->maxInternalNeurons;
            sourceId |= INTERNAL_BASE;
        }

        uint16_t sinkId = gene->sinkId;
        if (gene->sinkIsOutput) {
            sinkId %= OUT_MAX;
            sinkId |= OUT_BASE;
        } else {
            sinkId %= sim->maxInternalNeurons;
            sinkId |= INTERNAL_BASE;
        }

        Neuron *source = findNeuronById(neuronBuffer, usedNeurons, sourceId);
        if (source == NULL) {
            source = &neuronBuffer[usedNeurons++];
            source->id = sourceId;
            source->type = gene->sourceIsInput ? NEURON_INPUT : NEURON_INTERNAL;
            source->state = 0.0f;
            source->inputs = 0;
            source->outputs = 1;
            source->inputsVisited = 0;
            source->outputsVisited = 0;
        } else {
            source->outputs++;
        }

        Neuron *sink = findNeuronById(neuronBuffer, usedNeurons, sinkId);
        if (sink == NULL) {
            sink = &neuronBuffer[usedNeurons++];
            sink->id = sinkId;
            sink->type = gene->sinkIsOutput ? NEURON_OUTPUT : NEURON_INTERNAL;
            sink->state = 0.0f;
            sink->inputs = 1;
            sink->outputs = 0;
            sink->inputsVisited = 0;
            sink->outputsVisited = 0;
        } else {
            sink->inputs++;
        }

        NeuralConnection *conn = &connectionBuffer[usedConnections++];
        conn->sourceId = source->id;
        conn->sinkId = sink->id;
        conn->weight =
            (float)gene->weight * 8.0f / 65536.0f - 4.0f; // between -4.0 and +4.0
        conn->visited = false;
    }

    NeuralNet net;

    net.connectionCount = usedConnections;
    net.connections = connectionBuffer;

    net.neuronCount = usedNeurons;
    net.neurons = neuronBuffer;

    return net;
}

void destroyNeuralNet(NeuralNet *net)
{
    net->connections = NULL;
    net->neurons = NULL;
    net->connectionCount = 0;
    net->neuronCount = 0;
}

// make a deep copy of the neural net
NeuralNet copyNeuralNet(NeuralNet* src, Neuron* neuronBuffer, NeuralConnection* connectionBuffer)
{
    NeuralNet dest = *src;

    dest.connections = connectionBuffer;
    memcpy(dest.connections, src->connections, src->connectionCount * sizeof(NeuralConnection));

    dest.neurons = neuronBuffer;
    memcpy(dest.neurons, src->neurons, src->neuronCount * sizeof(Neuron));

    return dest;
}