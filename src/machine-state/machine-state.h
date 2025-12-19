#ifndef machine_state
#define machine_state

#include <stdbool.h>

#define ADDRESS_SPACE_SIZE 0x2000
#define IO_INTERFACE_ADDRESS 0x1fff
#define TIME_INTERFACE_ADDRESS 0x1ffb

struct MachineState {
    bool isUnconditionalInfiniteLoop;
    unsigned char memory[ADDRESS_SPACE_SIZE];
    unsigned short PC;
    unsigned char A;
    unsigned long simulationStartTimeMs;
    unsigned long simulationMeasuredTimeMs;
    unsigned long simulationIdleTimeMs;
};

struct MachineState getInitialState();

unsigned short peekInstruction(struct MachineState* state, unsigned short address);

unsigned short getInstruction(struct MachineState* state, unsigned short address);

unsigned char peekMemory(struct MachineState* state, unsigned short address);

unsigned char getMemory(struct MachineState* state, unsigned short address);

void step(struct MachineState* state);

#endif