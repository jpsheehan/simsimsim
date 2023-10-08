#ifndef Simulator_h
#define Simulator_h

#include "Common.h"

void runSimulation(SharedThreadState*);
void simSendReady(void);
void simSendQuit(void);

void simSendPause(void);
void simSendContinue(void);

void simSendFramePause(void);
void simSendFrameContinue(void);

#endif