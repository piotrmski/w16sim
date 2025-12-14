#ifndef machine_state
#define machine_state

#include <stdbool.h>

struct MachineState {
    bool isRunning;
    unsigned char memory[0x2000];
    unsigned short PC;
    unsigned char A;
};

struct MachineState getInitialState();

void step(struct MachineState* state);

#endif