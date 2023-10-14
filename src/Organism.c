#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Organism.h"
#include "Direction.h"
#include "Common.h"
#include "Geometry.h"
#include "NeuralNet.h"
#include "Genome.h"

#define SIM_COLLISION_DEATHS false

static char *InputTypeStrings[IN_MAX] = {"WORLD_X",
                                         "WORLD_Y",
                                         "IN_AGE",
                                         "IN_COLLIDE",
                                         "IN_ENERGY",
                                         "VISION_FORWARD",
                                         "IN_PROXIMITY_TO_NEAREST_EDGE"
                                        };

static char *OutputTypeStrings[OUT_MAX] = {
    "MOVE_X",          "MOVE_Y",     "MOVE_RANDOM", "MOVE_FORWARD_BACKWARD",
    "TURN_LEFT_RIGHT", "TURN_RANDOM"
};

Organism makeOffspring(Organism *a, Organism *b, Simulation* sim, Organism **orgsByPosition)
{
    Organism org = {
        .pos =
        (Pos)
        {
            .x = rand() % sim->size.w,
            .y = rand() % sim->size.h,
        },
        .alive = true,
        .didCollide = false,
        .energyLevel = 1.0,
        .direction = getRandomDirection()
    };

    org.genome = mutateGenome(reproduce(&a->genome, &b->genome), sim->mutationRate, &org.mutated);

    while (getOrganismByPos(org.pos, sim, orgsByPosition, false) != NULL ||
            isPosInAnyRect(org.pos, sim->obstacles, sim->obstaclesCount)) {
        org.pos.x = rand() % sim->size.w;
        org.pos.y = rand() % sim->size.h;
    }

    org.net = buildNeuralNet(&org.genome, sim);
    org.parentA = a->id;
    org.parentB = b->id;

    return org;
}

void destroyOrganism(Organism *org)
{
    destroyNeuralNet(&org->net);
    free(org->genome.genes);
    org->genome.genes = NULL;
    org->genome.count = 0;
}

void dumpOrganismNet(Organism *org)
{
    printf("BREAKDOWN OF NET:\n");
    for (int i = 0; i < org->net.connectionCount; i++) {
        NeuralConnection *connection = &org->net.connections[i];

        printf("Connection %i:\n", i);
        printf("  Weight = %.2f\n", connection->weight);
        printf("  Source = #%d\n", connection->sourceId);
        printf("  Sink   = #%d\n", connection->sinkId);
    }

    for (int i = 0; i < org->net.neuronCount; i++) {
        Neuron *neuron = &org->net.neurons[i];

        printf("Neuron %d:\n", i);
        printf("  ID      = %d\n", neuron->id);
        printf("  Type    = %s\n",
               (neuron->type == NEURON_INPUT
                ? "INPUT"
                : (neuron->type == NEURON_OUTPUT ? "OUTPUT" : "INTERNAL")));

        if (neuron->type == NEURON_INPUT)
            printf("  Type 2  = %s\n", InputTypeStrings[neuron->id & 0x7f]);
        if (neuron->type == NEURON_OUTPUT)
            printf("  Type 2  = %s\n", OutputTypeStrings[neuron->id & 0x7f]);

        printf("  Inputs  = %d\n", neuron->inputs);
        printf("  Outputs = %d\n", neuron->outputs);
    }
    printf("\n");
}

void findMates(Organism orgs[], int population, Organism **outA,
               Organism **outB)
{
    // finds two distinct organisms that are alive
    *outA = NULL;
    *outB = NULL;

    if (population == 0) {
        fprintf(stderr, "Cannot call findMates with a population of 0!\n");
        exit(1);
        return;
    }

    while (*outA == NULL) {
        *outA = &orgs[rand() % population];
        if (!(*outA)->alive) {
            *outA = NULL;
        }
    }

    while (*outB == NULL && *outA != *outB) {
        *outB = &orgs[rand() % population];
        if (!(*outB)->alive) {
            *outB = NULL;
        }
    }
}

bool inRange(int minInclusive, int x, int maxExclusive)
{
    return x >= minInclusive && x < maxExclusive;
}

Organism *getOrganismByPos(Pos pos, Simulation* sim, Organism** orgsByPosition,
                           bool aliveOnly)
{
    if (!inRange(0, pos.x, sim->size.w) ||
            !inRange(0, pos.y, sim->size.h)) {
        return NULL;
    }

    Organism* org = orgsByPosition[pos.y * sim->size.w + pos.x];

    if (org == NULL) {
        return NULL;
    }

    if ((aliveOnly && org->alive) || !aliveOnly) {
        return org;
    }

    return NULL;
}

// Sets the organism's position in the LUT if it is alive.
void setOrganismByPosition(Simulation* sim, Organism** orgsByPosition, Organism* org)
{
    if (!org->alive) return;

    orgsByPosition[sim->size.w * org->pos.y + org->pos.x] = org;
}

void organismMoveBackIntoZone(Organism *org, Simulation* sim)
{
    if (org->pos.x >= sim->size.w) {
        org->pos.x = sim->size.w - 1;
    } else if (org->pos.x < 0) {
        org->pos.x = 0;
    }
    if (org->pos.y >= sim->size.h) {
        org->pos.y = sim->size.h - 1;
    } else if (org->pos.y < 0) {
        org->pos.y = 0;
    }
}

void resetNeuronState(Organism* org)
{
    for (int i = 0; i < org->net.connectionCount; i++) {
        org->net.connections[i].visited = false;
        org->didCollide = false;
    }
    for (int i = 0; i < org->net.neuronCount; i++) {
        org->net.neurons[i].inputsVisited = 0;
        org->net.neurons[i].outputsVisited = 0;
        org->net.neurons[i].prevState = org->net.neurons[i].state;
        org->net.neurons[i].state = 0.0f;
    }
}

void exciteInputNeurons(Simulation* sim, Organism** prevOrgsByPosition, Organism* org, int currentStep)
{
    for (int i = 0; i < org->net.neuronCount; i++) {
        Neuron *input = &org->net.neurons[i];
        if (input->type != NEURON_INPUT) {
            continue;
        }

        switch (input->id & 0xff) {
        case IN_WORLD_X:
            input->state = 2.0 * (float)org->pos.x / (float)sim->size.w - 1.0;
            break;
        case IN_WORLD_Y:
            input->state = 2.0 * (float)org->pos.y / (float)sim->size.h - 1.0;
            break;
        case IN_AGE:
            input->state = 2.0 * (float)currentStep / (float)sim->stepsPerGeneration - 1.0;
            break;
        case IN_COLLIDE:
            input->state = org->didCollide ? 1.0 : 0.0;
            break;
        case IN_ENERGY:
            input->state = org->energyLevel;
            break;
        case IN_VISION_FORWARD:
            // TODO: this won't work since we need to split the organism run step into
            // two stages... the inputs should operate on the old state and the
            // outputs should operate on the new state
            if (getOrganismByPos(
                        addPos(org->pos, moveInDirection(org->pos, org->direction)),
                        sim, prevOrgsByPosition, true)) {
                input->state = 1.0f;
            } else {
                input->state = 0.0f;
            }
            break;
        case IN_PROXIMITY_TO_NEAREST_EDGE: {
            int nearX = sim->size.w / 2 - abs(sim->size.w / 2 - org->pos.x);
            int nearY = sim->size.h / 2 - abs(sim->size.h / 2 - org->pos.y);
            input->state = (nearX < nearY ? 1.0f - 2.0f * (float)nearX / sim->size.w
                            : 1.0f - 2.0f * (float)nearY / sim->size.h);
        }
        break;
        }

        if (fabs(input->state) > 1.0f)
            printf("Excited neuron #%d to level %.2f\n", input->id, input->state);
    }
}

void computeNeuronStates(Organism* org)
{
    int visits;
    do {
        visits = 0;

        for (int i = 0; i < org->net.connectionCount; i++) {
            NeuralConnection *connection = &org->net.connections[i];

            // if the connection has already been visited then skip
            if (connection->visited) {
                continue;
            }

            Neuron *source = findNeuronById(org->net.neurons, org->net.neuronCount,
                                            connection->sourceId);

            // if the source has inputs and they haven't been fulfilled then don't do
            // anything here yet
            if (source->inputs && (source->inputsVisited < source->inputs)) {
                continue;
            }

            Neuron *sink = findNeuronById(org->net.neurons, org->net.neuronCount,
                                          connection->sinkId);

            if (sink == source) {
                sink->state += connection->weight * source->prevState;
            } else {
                sink->state += connection->weight * source->state;
            }

            sink->inputsVisited++;
            source->outputsVisited++;
            connection->visited = true;
            // printf("Visited connection between neuron #%d and neuron #%d\n",
            // source->id, sink->id);

            // normalise the sink state if it has been completed between -1.0 and 1.0.
            if (sink->inputsVisited == sink->inputs) {
                sink->state = tanhf(sink->state / (float)sink->inputs);
                // printf("Normalised neuron #%d state to %.2f\n", sink->id,
                // sink->state);
            }

            visits++;
            // printf("Source has %d of %d inputs fulfilled\n", source->inputsVisited,
            // source->inputs);
        }
    } while (visits > 0);
}

void performNeuronOutputs(Organism* org, Pos originalPosition, Organism** orgsByPosition, Simulation* sim)
{
    bool didMove = false;
    for (int i = 0; i < org->net.neuronCount; i++) {
        Neuron *output = &org->net.neurons[i];

        if (output->type != NEURON_OUTPUT) {
            // printf("Neuron #%d is not an output\n", output->id);
            continue;
        }

        // printf("Output neuron #%d has level %.2f\n", output->id, output->state);

        switch (output->id & 0xff) {
        case OUT_MOVE_X:
            if (output->state >= 0.5f) {
                org->pos.x++;
                didMove = true;
            } else if (output->state <= -0.5f) {
                org->pos.x--;
                didMove = true;
            }
            break;
        case OUT_MOVE_Y:
            if (output->state >= 0.5f) {
                org->pos.y++;
                didMove = true;
            } else if (output->state <= -0.5f) {
                org->pos.y--;
                didMove = true;
            }
            break;
        case OUT_MOVE_RANDOM:
            if (fabs(output->state) >= 0.5f) {
                org->pos.x += rand() % 2 == 0 ? -1 : 1;
                org->pos.y += rand() % 2 == 0 ? -1 : 1;
                didMove = true;
            }
            break;
        case OUT_MOVE_FORWARD_BACKWARD:
            if (output->state >= 0.5f) {
                org->pos = moveInDirection(org->pos, org->direction);
                didMove = true;
            } else if (fabs(output->state) <= 0.5f) {
                org->pos = moveInDirection(org->pos, turnBackwards(org->direction));
                didMove = true;
            }
            break;
        case OUT_TURN_LEFT_RIGHT:
            if (output->state <= 0.5f) {
                org->direction = turnLeft(org->direction);
            } else if (output->state >= 0.5f) {
                org->direction = turnRight(org->direction);
            }
            break;
        case OUT_TURN_RANDOM:
            if (fabs(output->state) >= 0.5f) {
                org->direction =
                    rand() % 2 ? turnLeft(org->direction) : turnRight(org->direction);
            }
            break;
        }
    }

    if (didMove) {
        org->energyLevel -= sim->energyToMove;
    } else {
        org->energyLevel += sim->energyToRest;
    }

    if (org->energyLevel <= 0.0f) {
        org->alive = false;
        org->energyLevel = 0.0f;
        org->pos = originalPosition;
        setOrganismByPosition(sim, orgsByPosition, org);
    } else if (org->energyLevel > 1.0f) {
        org->energyLevel = 1.0f;
    }
}

void handleCollisions(Organism* org, Simulation* sim, Organism** orgsByPosition, Organism** prevOrgsByPosition)
{
    // collisions
    organismMoveBackIntoZone(org, sim);

#if SIM_COLLISION_DEATHS
    if (getOrganismByPos(org->pos, otherOrgs, otherOrgsCount, true) ||
            isPosInAnyRect(org->pos, sim->obstacles, sim->obstaclesCount)) {
        org->didCollide = true;
        org->pos = originalPosition;
        if (getOrganismByPos(org->pos, otherOrgs, otherOrgsCount, true) ||
                isPosInAnyRect(org->pos, sim->obstacles, sim->obstaclesCount)) {
            org->alive = false;
            setOrganismByPosition(sim, orgsByPosition, org);
            return;
        }
    }
#else
    Organism* collidedOrg;
    while ((collidedOrg = getOrganismByPos(org->pos, sim, prevOrgsByPosition, true)) ||
            isPosInAnyRect(org->pos, sim->obstacles, sim->obstaclesCount)) {
        if (collidedOrg == org) break;
        org->didCollide = true;
        org->pos.x += rand() % 3 - 1;
        org->pos.y += rand() % 3 - 1;
        organismMoveBackIntoZone(org, sim);
    }

    setOrganismByPosition(sim, orgsByPosition, org);
#endif
}

void organismRunStep(Organism *org, Organism **orgsByPosition, Organism** prevOrgsByPosition, Simulation* sim, int currentStep)
{
    if (!org->alive)
        return;

    Pos originalPosition = org->pos;

    resetNeuronState(org);

    exciteInputNeurons(sim, orgsByPosition, org, currentStep);

    computeNeuronStates(org);

    performNeuronOutputs(org, originalPosition, orgsByPosition, sim);

    if (!org->alive)
        return;

    handleCollisions(org, sim, orgsByPosition, prevOrgsByPosition);
}

Organism makeRandomOrganism(Simulation* sim, Organism** orgsByPosition)
{
    Organism org = {
        .pos = (Pos){.x = rand() % sim->size.w, .y = rand() % sim->size.h},
        .genome = makeRandomGenome(sim->numberOfGenes),
        .alive = true,
        .didCollide = false,
        .energyLevel = 1.0,
        .direction = getRandomDirection(),
        .mutated = false,
    };

    while (getOrganismByPos(org.pos, sim, orgsByPosition, false) ||
            isPosInAnyRect(org.pos, sim->obstacles, sim->obstaclesCount)) {
        org.pos.x = rand() % sim->size.w;
        org.pos.y = rand() % sim->size.h;
    }

    org.net = buildNeuralNet(&org.genome, sim);

    return org;
}

/// Makes a deep copy of the organism; this new Organism will need to be destroyed independently of its original.
Organism copyOrganism(Organism *src)
{
    Organism dest = *src;

    dest.genome = copyGenome(&src->genome);
    dest.net = copyNeuralNet(&src->net);

    return dest;
}

// copies just the organisms state that can change between steps
void copyOrganismMutableState(Organism* dest, Organism* src)
{
    if (dest == NULL || src == NULL) return;

    dest->alive = src->alive;
    dest->didCollide = src->didCollide;
    dest->direction = src->direction;
    dest->energyLevel = src->energyLevel;
    dest->pos = src->pos;

    for (int i = 0; i < src->net.neuronCount; i++) {
        dest->net.neurons[i].prevState = src->net.neurons[i].prevState;
        dest->net.neurons[i].state = src->net.neurons[i].state;
    }
}