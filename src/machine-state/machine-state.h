#ifndef machine_state
#define machine_state

#include <stdbool.h>

struct MachineState {
    bool isUnconditionalInfiniteLoop;
    unsigned char memory[0x2000];
    unsigned short PC;
    unsigned char A;
};

struct MachineState getInitialState();

unsigned short getCurrentInstruction(struct MachineState* state);

void step(struct MachineState* state);

#endif