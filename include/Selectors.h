#ifndef Selectors_h
#define Selectors_h

#include "Common.h"

extern SelectionCriteria topHalfSelector;
extern SelectionCriteria bottomHalfSelector;
extern SelectionCriteria leftHalfSelector;
extern SelectionCriteria rightHalfSelector;

extern SelectionCriteria centerSelector;
extern SelectionCriteria centerXSelector;
extern SelectionCriteria centerYSelector;
extern SelectionCriteria circleCenterSelector;

extern SelectionCriteria farLeftSelector;
extern SelectionCriteria farRightSelector;
extern SelectionCriteria farTopSelector;
extern SelectionCriteria farBottomSelector;

extern SelectionCriteria farTopOrBottomSelector;
extern SelectionCriteria farLeftOrRightSelector;

// bool farLeftSelector(Organism *org, Simulation *sim);
// bool farRightSelector(Organism *org, Simulation *sim);

// bool farLeftAndRightSelector(Organism *org, Simulation *sim);

#endif
