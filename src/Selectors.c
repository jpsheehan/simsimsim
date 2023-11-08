#include "Selectors.h"

bool centerXSelectorFn(Organism *org, Simulation *sim)
{
    return ((org->pos.x > (sim->size.w / 3)) && (org->pos.x < (2 * sim->size.w / 3)));
}

const char centerXSelectorName[] = "Center (x)";

SelectionCriteria centerXSelector = {
    .fn = centerXSelectorFn,
    .name = centerXSelectorName,
};

bool centerYSelectorFn(Organism *org, Simulation *sim)
{
    return ((org->pos.y > (sim->size.h / 3)) &&
            (org->pos.y < (2 * sim->size.h / 3)));
}

const char centerYSelectorName[] = "Center (y)";

SelectionCriteria centerYSelector = {
    .fn = centerYSelectorFn,
    .name = centerYSelectorName,
};

bool centerSelectorFn(Organism *org, Simulation *sim)
{
    return centerXSelectorFn(org, sim) && centerYSelectorFn(org, sim);
}

const char centerSelectorName[] = "Center (Square)";

SelectionCriteria centerSelector = {
    .fn = centerSelectorFn,
    .name = centerSelectorName,
};

bool circleCenterSelectorFn(Organism* org, Simulation *sim)
{
    return
        ((sim->size.w / 2 - org->pos.x) * (sim->size.w / 2 - org->pos.x) +
         (sim->size.h / 2 - org->pos.y) * (sim->size.h / 2 - org->pos.y)) < (20 * 20);
}

const char circleCenterSelectorName[] = "Center (Circle)";

SelectionCriteria circleCenterSelector = {
    .fn = circleCenterSelectorFn,
    .name = circleCenterSelectorName,
};

bool hollowCircleSelectorFn(Organism* org, Simulation *sim)
{
    return
        ((sim->size.w / 2 - org->pos.x) * (sim->size.w / 2 - org->pos.x) +
         (sim->size.h / 2 - org->pos.y) * (sim->size.h / 2 - org->pos.y)) > (60 * 60);
}

const char hollowCircleSelectorName[] = "Hollow Circle";

SelectionCriteria hollowCircleSelector = {
    .fn = hollowCircleSelectorFn,
    .name = hollowCircleSelectorName,
};


bool donutSelectorFn(Organism* org, Simulation *sim)
{
    return
        (((sim->size.w / 2 - org->pos.x) * (sim->size.w / 2 - org->pos.x) +
         (sim->size.h / 2 - org->pos.y) * (sim->size.h / 2 - org->pos.y)) < (32 * 32)) &&
        ((sim->size.w / 2 - org->pos.x) * (sim->size.w / 2 - org->pos.x) +
         (sim->size.h / 2 - org->pos.y) * (sim->size.h / 2 - org->pos.y)) > (24 * 24);
}

const char donutSelectorName[] = "Donut";

SelectionCriteria donutSelector = {
    .fn = donutSelectorFn,
    .name = donutSelectorName,
};


bool bottomHalfSelectorFn(Organism *org, Simulation *sim)
{
    return (org->pos.y >= (sim->size.h / 2));
}

const char bottomHalfSelectorName[] = "Bottom Half";

SelectionCriteria bottomHalfSelector = {
    .fn = bottomHalfSelectorFn,
    .name = bottomHalfSelectorName,
};

bool topHalfSelectorFn(Organism *org, Simulation *sim)
{
    return (org->pos.y < (sim->size.h / 2));
}

const char topHalfSelectorName[] = "Top Half";

SelectionCriteria topHalfSelector = {
    .fn = topHalfSelectorFn,
    .name = topHalfSelectorName,
};

bool leftHalfSelectorFn(Organism* org, Simulation *sim)
{
    return (org->pos.x < (sim->size.h / 2));
}

const char leftHalfSelectorName[] = "Left Half";

SelectionCriteria leftHalfSelector = {
    .fn = leftHalfSelectorFn,
    .name = leftHalfSelectorName,
};

bool rightHalfSelectorFn(Organism* org, Simulation *sim)
{
    return (org->pos.x > (sim->size.h / 2));
}

const char rightHalfSelectorName[] = "Right Half";

SelectionCriteria rightHalfSelector = {
    .fn = rightHalfSelectorFn,
    .name = rightHalfSelectorName,
};

bool farLeftSelectorFn(Organism *org, Simulation *sim)
{
    return org->pos.x < sim->size.w * 0.2;
}

const char farLeftSelectorName[] = "Left 20%";

SelectionCriteria farLeftSelector = {
    .fn = farLeftSelectorFn,
    .name = farLeftSelectorName,
};

bool farRightSelectorFn(Organism *org, Simulation *sim)
{
    return org->pos.x > sim->size.w * 0.8;
}

const char farRightSelectorName[] = "Right 20%";

SelectionCriteria farRightSelector = {
    .fn = farRightSelectorFn,
    .name = farRightSelectorName,
};

bool farLeftOrRightSelectorFn(Organism *org, Simulation *sim)
{
    return farLeftSelectorFn(org, sim) || farRightSelectorFn(org, sim);
}

const char farLeftOrRightSelectorName[] = "L/R Edges";

SelectionCriteria farLeftOrRightSelector = {
    .fn = farLeftOrRightSelectorFn,
    .name = farLeftOrRightSelectorName,
};