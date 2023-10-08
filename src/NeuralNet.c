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

NeuralNet buildNeuralNet(Genome *genome, Simulation* sim)
{
    Neuron neurons[128] = {0};
    uint8_t usedNeurons = 0;

    NeuralConnection connections[128] = {0};
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

        Neuron *source = findNeuronById(neurons, usedNeurons, sourceId);
        if (source == NULL) {
            source = &neurons[usedNeurons++];
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

        Neuron *sink = findNeuronById(neurons, usedNeurons, sinkId);
        if (sink == NULL) {
            sink = &neurons[usedNeurons++];
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

        NeuralConnection *conn = &connections[usedConnections++];
        conn->sourceId = source->id;
        conn->sinkId = sink->id;
        conn->weight =
            (float)gene->weight * 8.0f / 65536.0f - 4.0f; // between -4.0 and +4.0
        conn->visited = false;
    }

    NeuralNet net;

    net.connectionCount = usedConnections;
    net.connections = calloc(usedConnections, sizeof(NeuralConnection));
    memcpy(net.connections, connections,
           usedConnections * sizeof(NeuralConnection));

    net.neuronCount = usedNeurons;
    net.neurons = calloc(usedNeurons, sizeof(Neuron));
    memcpy(net.neurons, neurons, usedNeurons * sizeof(Neuron));

    return net;
}

void destroyNeuralNet(NeuralNet *net)
{
    free(net->connections);
    free(net->neurons);
    net->connections = NULL;
    net->neurons = NULL;
    net->connectionCount = 0;
    net->neuronCount = 0;
}

// make a deep copy of the neural net
NeuralNet copyNeuralNet(NeuralNet* src)
{
    NeuralNet dest = *src;

    dest.connections = calloc(src->connectionCount, sizeof(NeuralConnection));
    memcpy(dest.connections, src->connections, src->connectionCount * sizeof(NeuralConnection));

    dest.neurons = calloc(src->neuronCount, sizeof(Neuron));
    memcpy(dest.neurons, src->neurons, src->neuronCount * sizeof(Neuron));

    return dest;
}