#ifndef machine_state
#define machine_state

#include <stdbool.h>

struct MachineState {
    bool isUnconditionalInfiniteLoop;
    unsigned char memory[0x2000];
    unsigned short PC;
    unsigned char A;
    unsigned long simulationStartTimeMs;
    unsigned long simulationMeasuredTimeMs;
};

struct MachineState getInitialState();

unsigned short peekCurrentInstruction(struct MachineState* state);

unsigned short getCurrentInstruction(struct MachineState* state);

unsigned char peekMemoryAtAddress(struct MachineState* state, unsigned short address);

unsigned char getMemoryAtAddress(struct MachineState* state, unsigned short address);

void step(struct MachineState* state);

#endif