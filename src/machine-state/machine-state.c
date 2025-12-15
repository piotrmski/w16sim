#include "machine-state.h"
#include <stdio.h>
#include "../keyboard-input/keyboard-input.h"

struct MachineState getInitialState()
{
    return (struct MachineState) { false, { 0 }, 0, 0 };
}

unsigned short getCurrentInstruction(struct MachineState* state) {
    return state->memory[state->PC] | ((state->PC == 0x1fff ? state->memory[0] : state->memory[state->PC + 1]) << 8);
}

void step(struct MachineState* state)
{
    unsigned short instruction = getCurrentInstruction(state);
    unsigned char opcode = instruction >> 13;
    unsigned short argument = instruction & 0x1fff;
    unsigned char memoryAtArgument = argument == 0x1fff ? getLastChar() : state->memory[argument]; // TODO add time support

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
            if (argument == 0x1fff) putchar(state->A);
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
