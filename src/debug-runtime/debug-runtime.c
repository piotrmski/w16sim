#include "debug-runtime.h"
#include "../machine-state/machine-state.h"
#include "../keyboard-input/keyboard-input.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>

#define LABEL_NAME_MAX_LENGTH 32

#define HELP_MESSAGE = "Commands:\n\
h     - prints this message\n\
l     - lists memory values from PC-5 to PC+5\n\
l X   - lists the memory value of X\n\
l X:Y - lists memory values from X to Y\n\
a     - lists all registered label names and their values\n\
r     - lists program counter value, instruction, memory value at instruction argument, and register A value\n\
b     - sets a breakpoint at PC\n\
b X   - sets a breakpoint at X\n\
d     - deletes a breakpoint at PC\n\
d X   - deletes a breakpoint at X\n\
c     - continues simulation\n\
s     - executes one instruction and pauses\n\
q     - quits\n\
For a command parameter value you may use one of the following:\n\
- a hexadecimal number starting with \"0x\",\n\
- PC,\n\
- PC+C or PC-C where C is a decimal number constant,\n\
- a label name,\n\
- L+C or L-C where L is a label name and C is a decimal number constant."


static volatile bool isPaused = true;

static void handleSigInt(int _) {
    if (isPaused) {
        printf("\nQuitting\n");
        exit(0);
    } else {
        isPaused = true;
    }
}

static bool isPrintableChar(unsigned char ch) {
    return ch >= 32 && ch <= 126;
}

static void interactivePrompt(struct MachineState* state) {
    printf("\nPaused.  PC = 0x%04X   M[PC] = 0x%02X   A = 0x%02X\n", state->PC, peekMemoryAtAddress(state, state->PC), state->A);
    bool interactive = true;
    while (interactive) {
        printf("> ");
        char command[128] = {0};
        fgets(command, 127, stdin);
        if (command[0] == 'c') {
            interactive = false;
        }
    }
}

void runDebug(struct MachineState* state, const char* symbolsFilePath) {
    printf("Starting in debug mode. Type 'h' to list all commands or 'c' to begin simulation. Press ^C during simulation to pause.");

    signal(SIGINT, handleSigInt);

    startCharacterInput();

    do {
        if (isPaused) {
            endCharacterInput();
            interactivePrompt(state);
            startCharacterInput();
        } else {
            step(state);
        }
    } while (!state->isUnconditionalInfiniteLoop);

    endCharacterInput();

    printf("Unconditional infinite loop detected. Ending simulation.\n");
}
