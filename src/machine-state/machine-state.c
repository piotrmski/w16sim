#include "machine-state.h"
#include "../keyboard-input/keyboard-input.h"
#include "../time/time.h"
#include <stdio.h>

struct MachineState getInitialState()
{
    unsigned long now = getTimeMs();
    return (struct MachineState) { false, { 0 }, 0, 0, now, now, 0 };
}

unsigned short peekInstruction(struct MachineState* state, unsigned short address) {
    return peekMemory(state, address) | (peekMemory(state, address + 1) << 8);
}

unsigned short getInstruction(struct MachineState* state, unsigned short address) {
    return getMemory(state, address) | (getMemory(state, address + 1) << 8);
}

unsigned char peekMemory(struct MachineState* state, unsigned short address) {
    address %= 0x2000; 

    switch (address) {
        case IO_INTERFACE_ADDRESS:
            return peekLastChar();
        case TIME_INTERFACE_ADDRESS:
        case TIME_INTERFACE_ADDRESS + 1:
        case TIME_INTERFACE_ADDRESS + 2:
        case TIME_INTERFACE_ADDRESS + 3:
            unsigned long timeDifference = state->simulationMeasuredTimeMs - state->simulationStartTimeMs - state->simulationIdleTimeMs;
            int offset = (address - TIME_INTERFACE_ADDRESS) * 8;
            return timeDifference >> offset;
        default:
            return state->memory[address];
    }
}

unsigned char getMemory(struct MachineState* state, unsigned short address) {
    switch (address) {
        case IO_INTERFACE_ADDRESS:
            return getLastChar();
        case TIME_INTERFACE_ADDRESS:
            state->simulationMeasuredTimeMs = getTimeMs();
        default:
            return peekMemory(state, address);
    }
}

void step(struct MachineState* state)
{
    unsigned short instruction = getInstruction(state, state->PC);
    unsigned char opcode = instruction >> 13;
    unsigned short argument = instruction & 0x1fff;
    unsigned char memoryAtArgument = getMemory(state, argument);

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
            if (argument == IO_INTERFACE_ADDRESS) {
                putchar(state->A);
                fflush(stdout);
            }
            else state->memory[argument] = state->A;
            state->PC += 2;
            break;
        case 5: // JMP
            if (state->PC == argument) state->isUnconditionalInfiniteLoop = true;
            else state->PC = argument;
            break;
        case 6: // JMN
            state->PC = (state->A & 0x80) ? argument : state->PC + 2;
            break;
        case 7: // JMZ
            state->PC = state->A == 0 ? argument : state->PC + 2;
            break;
    }
    
    state->PC %= 0x2000;
}
