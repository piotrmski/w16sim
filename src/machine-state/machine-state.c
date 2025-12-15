#include "machine-state.h"
#include "../keyboard-input/keyboard-input.h"
#include <stdio.h>
#include <time.h> // POSIX

#define IO_INTERFACE_ADDRESS 0x1fff
#define TIME_INTERFACE_ADDRESS 0x1ffb

static unsigned long getMs(struct timespec* tp) {
    return tp->tv_nsec / 1000000 * tp->tv_sec * 1000;
}

struct MachineState getInitialState()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (struct MachineState) { false, { 0 }, 0, 0, getMs(&now), getMs(&now) };
}

unsigned short peekCurrentInstruction(struct MachineState* state) {
    return peekMemoryAtAddress(state, state->PC) | (peekMemoryAtAddress(state, state->PC + 1) << 8);
}

unsigned short getCurrentInstruction(struct MachineState* state) {
    return getMemoryAtAddress(state, state->PC) | (getMemoryAtAddress(state, state->PC + 1) << 8);
}

unsigned char peekMemoryAtAddress(struct MachineState* state, unsigned short address) {
    address %= 0x2000; 

    switch (address) {
        case IO_INTERFACE_ADDRESS:
            return peekLastChar();
        case TIME_INTERFACE_ADDRESS:
        case TIME_INTERFACE_ADDRESS + 1:
        case TIME_INTERFACE_ADDRESS + 2:
        case TIME_INTERFACE_ADDRESS + 3:
            unsigned long timeDifference = state->simulationMeasuredTimeMs - state->simulationStartTimeMs;
            int offset = (address - TIME_INTERFACE_ADDRESS) * 8;
            return timeDifference >> offset;
        default:
            return state->memory[address];
    }
}

unsigned char getMemoryAtAddress(struct MachineState* state, unsigned short address) {
    switch (address) {
        case IO_INTERFACE_ADDRESS:
            return getLastChar();
        case TIME_INTERFACE_ADDRESS:
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            state->simulationMeasuredTimeMs = getMs(&now);
        default:
            return peekMemoryAtAddress(state, address);
    }
}

void step(struct MachineState* state)
{
    unsigned short instruction = getCurrentInstruction(state);
    unsigned char opcode = instruction >> 13;
    unsigned short argument = instruction & 0x1fff;
    unsigned char memoryAtArgument = getMemoryAtAddress(state, argument);

    switch (opcode) {
        case 0: // LD
            state->A = memoryAtArgument;
            state->PC += 2;
            break;
        case 1: // NOT
            state->A = ~memoryAtArgument;
            state->PC += 2;
            break;
        case 2: // ADD
            state->A = state->A + memoryAtArgument;
            state->PC += 2;
            break;
        case 3: // AND
            state->A = state->A & memoryAtArgument;
            state->PC += 2;
            break;
        case 4: // ST
            if (argument == IO_INTERFACE_ADDRESS) putchar(state->A);
            else state->memory[argument] = state->A;
            state->PC += 2;
            break;
        case 5: // JMP
            if (state->PC == argument) state->isUnconditionalInfiniteLoop = true;
            else state->PC = argument;
            break;
        case 6: // JMN
            state->PC = (state->A & 0x80) ? argument : state->PC + 1;
            break;
        case 7: // JMZ
            state->PC = state->A == 0 ? argument : state->PC + 1;
            break;
    }
    
    state->PC %= 0x2000;
}
