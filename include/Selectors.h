#ifndef Selectors_h
#define Selectors_h

#include "Common.h"

bool centerXSelector(Organism *org, Simulation *sim);
bool centerYSelector(Organism *org, Simulation *sim);

bool collidedSelector(Organism *org, Simulation *sim);

bool hasEnoughEnergySelector(Organism *org, Simulation *sim);

bool centerSelector(Organism *org, Simulation *sim);
bool circleCenterSelector(Organism* org, Simulation *sim);

bool triangleSelector(Organism *org, Simulation *sim);

bool topHalfSelector(Organism *org, Simulation *sim);
bool bottomHalfSelector(Organism *org, Simulation *sim);

bool farLeftSelector(Organism *org, Simulation *sim);
bool farRightSelector(Organism *org, Simulation *sim);

bool farLeftAndRightSelector(Organism *org, Simulation *sim);

#endif
