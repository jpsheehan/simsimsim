#include "Selectors.h"

bool centerXSelector(Organism *org, Simulation *sim)
{
    return ((org->pos.x > (sim->size.w / 3)) && (org->pos.x < (2 * sim->size.w / 3)));
}

bool centerYSelector(Organism *org, Simulation *sim)
{
    return ((org->pos.y > (sim->size.h / 3)) &&
            (org->pos.y < (2 * sim->size.h / 3)));
}

bool collidedSelector(Organism *org, Simulation *sim)
{
    return org->didCollide;
}

bool hasEnoughEnergySelector(Organism *org, Simulation *sim)
{
    return org->energyLevel > 0.7f;
}

bool centerSelector(Organism *org, Simulation *sim)
{
    return centerXSelector(org, sim) && centerYSelector(org, sim);
}

bool circleCenterSelector(Organism* org, Simulation *sim)
{
    return
        ((sim->size.w / 2 - org->pos.x) * (sim->size.w / 2 - org->pos.x) +
         (sim->size.h / 2 - org->pos.y) * (sim->size.h / 2 - org->pos.y)) < (16 * 16);
}

bool triangleSelector(Organism *org, Simulation *sim)
{
    return (org->pos.x >= org->pos.y);
}

bool bottomHalfSelector(Organism *org, Simulation *sim)
{
    return (org->pos.y >= (sim->size.h / 2));
}

bool topHalfSelector(Organism *org, Simulation *sim)
{
    return (org->pos.y < (sim->size.h / 2));
}

bool farLeftSelector(Organism *org, Simulation *sim)
{
    return org->pos.x < sim->size.w * 0.2;
}
bool farRightSelector(Organism *org, Simulation *sim)
{
    return org->pos.x > sim->size.w * 0.8;
}

bool farLeftAndRightSelector(Organism *org, Simulation *sim)
{
    return farLeftSelector(org, sim) || farRightSelector(org, sim);
}