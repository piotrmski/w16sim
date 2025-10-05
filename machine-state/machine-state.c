#include "machine-state.h"
#include <stdio.h>
#include "../keyboard-input/keyboard-input.h"

struct MachineState getInitialState()
{
    return (struct MachineState) { true, { 0 }, 0, 0 };
}

void step(struct MachineState* state)
{
    unsigned short instruction = state->memory[state->PC] >> 13;
    unsigned short argument = state->memory[state->PC] & 0x1fff;
    unsigned short memoryAtArgument = argument == 0x1fff ? getLastChar() : state->memory[argument];

    switch (instruction) {
        case 0: // LD
            state->A = memoryAtArgument;
            ++state->PC;
            break;
        case 1: // NOT
            state->A = ~memoryAtArgument;
            ++state->PC;
            break;
        case 2: // ADD
            state->A = state->A + memoryAtArgument;
            ++state->PC;
            break;
        case 3: // AND
            state->A = state->A & memoryAtArgument;
            ++state->PC;
            break;
        case 4: // ST
            if (argument == 0x1fff) putchar(state->A);
            else state->memory[argument] = state->A;
            ++state->PC;
            break;
        case 5: // JMP
            if (state->PC == argument) state->isRunning = false;
            else state->PC = argument;
            break;
        case 6: // JMN
            state->PC = (state->A & 0x8000) ? argument : state->PC + 1;
            break;
        case 7: // JMZ
            state->PC = state->A == 0 ? argument : state->PC + 1;
            break;
    }
    
    state->PC %= 0x2000;
}
