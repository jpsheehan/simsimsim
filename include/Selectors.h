#ifndef Selectors_h
#define Selectors_h

#include "Common.h"

bool bottomSelector(Organism *org, Simulation *sim);
bool centerXSelector(Organism *org, Simulation *sim);
bool centerYSelector(Organism *org, Simulation *sim);
bool collidedSelector(Organism *org, Simulation *sim);
bool hasEnoughEnergySelector(Organism *org, Simulation *sim);
bool centerSelector(Organism *org, Simulation *sim);
bool circleCenterSelector(Organism* org, Simulation *sim);
bool triangleSelector(Organism *org, Simulation *sim);
bool leftSelector(Organism *org, Simulation *sim);
bool rightSelector(Organism *org, Simulation *sim);
bool leftAndRightSelector(Organism *org, Simulation *sim);

#endif
