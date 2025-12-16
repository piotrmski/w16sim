#include "debug-runtime.h"
#include "../machine-state/machine-state.h"
#include "../keyboard-input/keyboard-input.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#define LABEL_NAME_MAX_LENGTH 31

enum DataType {
    DataTypeInt = 0,
    DataTypeInstruction,
    DataTypeChar
};

static volatile bool isPaused = true;
static bool isStepping = false;
static char* labelNames[ADDRESS_SPACE_SIZE] = { NULL };
static bool breakpoints[ADDRESS_SPACE_SIZE] = { false };
static enum DataType dataTypes[ADDRESS_SPACE_SIZE] = { DataTypeInt, [IO_INTERFACE_ADDRESS] = DataTypeChar };

static char charUppercase(char ch) {
    if (ch >= 'a' && ch <= 'z') return ch - 0x20;
    else return ch;
}

static bool stringEqualCaseInsensitive(char* string1, char* string2) {
    for (int i = 0;; ++i) {
        if (charUppercase(string1[i]) != charUppercase(string2[i])) {
            return false;
        }

        if (string1[i] == 0) {
            return true;
        }
    }
}

static void parseSymbolsFile(char* path) {
    if (path == NULL) return;

    FILE* file = fopen(path, "r");

    if (file == NULL) {
        printf("Error: could not read file \"%s\".\n", path);
        exit(1);
    }

    char line[128] = {0};
    char* addressString;
    char* dataTypeString;
    char* labelName;
    int lineNumber = 0;
    static bool addressDescribed[ADDRESS_SPACE_SIZE] = { false };

    while (fgets(line, 127, file) != NULL) {
        ++lineNumber;

        addressString = strtok(line, " ,\t\n");
        dataTypeString = strtok(NULL, " ,\t\n");
        labelName = strtok(NULL, " ,\t\n");

        if (addressString == NULL) continue;
        
        if (dataTypeString == NULL) {
            printf("Error: in file \"%s\" line %d has too few columns.\n", path, lineNumber);
            exit(1);
        }

        int addressNumber = strtol(addressString, NULL, 0);
        if (errno != 0) {
            printf("Error: in file \"%s\" line %d: %s could not be parsed as a number.\n", path, lineNumber, addressString);
            exit(1);
        } else if (addressNumber < 0 || addressNumber >= ADDRESS_SPACE_SIZE) {
            printf("Error: in file \"%s\" line %d: address %s is out of range.\n", path, lineNumber, addressString);
            exit(1);
        } else if (addressDescribed[addressNumber]) {
            printf("Error: in file \"%s\" line %d: address 0x%04X was described multiple times.\n", path, lineNumber, addressNumber);
            exit(1);
        }
        addressDescribed[addressNumber] = true;

        enum DataType dataType;
        if (stringEqualCaseInsensitive(dataTypeString, "int")) {
            dataType = DataTypeInt;
        } else if (stringEqualCaseInsensitive(dataTypeString, "char")) {
            dataType = DataTypeChar;
        } else if (stringEqualCaseInsensitive(dataTypeString, "instruction")) {
            dataType = DataTypeInstruction;
        } else {
            printf("Error: in file \"%s\" line %d: unknown data type \"%s\".\n", path, lineNumber, dataTypeString);
            exit(1);
        }

        dataTypes[addressNumber] = dataType;

        if (labelName != NULL) {
            int labelNameLength = strlen(labelName);

            if (labelNameLength > LABEL_NAME_MAX_LENGTH) {
                printf("Error: in file \"%s\" line %d: label name must not be longer than %d characters.\n", path, lineNumber, LABEL_NAME_MAX_LENGTH);
                exit(1);
            }

            labelNames[addressNumber] = malloc(labelNameLength + 1);
            memcpy(labelNames[addressNumber], labelName, labelNameLength + 1);
        }
    }

    fclose(file);
}

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

static bool anyArgumentPresent(char* command) {
    for (int i = 1; command[i] != 0; ++i) {
        if (!isspace(command[i])) {
            printf("Command \"%c\" doesn't take any arguments. Type \"h\" to list all commands.\n", command[0]);
            return true;
        }
    }

    return false;
}

static void executeHelpCommand(char* command) {
    if (anyArgumentPresent(command)) return;

    printf("Commands:\n\
h     - prints this message\n\
l     - lists memory values from PC-3 to PC+3\n\
l X   - lists the memory value of X\n\
l X:Y - lists memory values from X to Y\n\
a     - lists all registered label names and their values\n\
r     - lists register A value, program counter value, and instruction at PC \n\
b     - sets a breakpoint at PC\n\
b X   - sets a breakpoint at X\n\
d     - deletes a breakpoint at PC\n\
d X   - deletes a breakpoint at X\n\
c     - continues simulation\n\
s     - steps simulation (executes one instruction and pauses)\n\
q     - quits\n\
For a command argument value you may use one of the following:\n\
- a hexadecimal number starting with \"0x\",\n\
- PC,\n\
- PC+C or PC-C where C is a decimal number constant,\n\
- a label name,\n\
- L+C or L-C where L is a label name and C is a decimal number constant.\n");
}

static void printInstruction(struct MachineState* state, int address) {
    unsigned short instruction = peekInstruction(state, address);
    unsigned char opcode = instruction >> 13;
    unsigned short argument = instruction & 0x1fff;

    switch (opcode) {
        case 0: // LD
            printf("LD ");
            break;
        case 1: // NOT
            printf("NOT ");
            break;
        case 2: // ADD
            printf("ADD ");
            break;
        case 3: // AND
            printf("AND ");
            break;
        case 4: // ST
            printf("ST ");
            break;
        case 5: // JMP
            printf("JMP ");
            break;
        case 6: // JMN
            printf("JMN ");
            break;
        case 7: // JMZ
            printf("JMZ ");
            break;
    }

    if (labelNames[argument] == NULL) {
        printf("0x%04X", argument);
    } else {
        printf("%s", labelNames[argument]);
    }

    if (opcode < 5) {
        unsigned char memoryValue = peekMemory(state, argument);

        if (labelNames[argument] == NULL) {
            printf("   M[0x%04X] = %d ", argument, memoryValue);
        } else if (strlen(labelNames[argument]) > 8) {
            printf("   M[%c%c%c%c%c...] = %d ", labelNames[argument][0], labelNames[argument][1], labelNames[argument][2], labelNames[argument][3], labelNames[argument][4], memoryValue);
        } else {
            printf("   M[%s] = %d ", labelNames[argument], memoryValue);
        }

        if (dataTypes[argument] == DataTypeChar && isPrintableChar(memoryValue)) {
            printf("'%c'", memoryValue);
        } else {
            printf("0x%02X", memoryValue);
        }
    }
}

static void executeListMemoryCommand(struct MachineState* state, char* command) {
    // TODO
}

static void executeListLabelsCommand(char* command) {
    if (anyArgumentPresent(command)) return;

    bool anyLabelDefined = false;
    for (int i = 0; i < ADDRESS_SPACE_SIZE; ++i) {
        if (labelNames[i] != NULL) {
            printf("0x%04X %s\n", i, labelNames[i]);
            anyLabelDefined = true;
        }
    }
    if (!anyLabelDefined) {
        printf("No labels defined\n");
    }
}

static void listRegisters(struct MachineState* state) {
    printf("A = %d", state->A);

    if (isPrintableChar(state->A)) {
        printf(" '%c'", state->A);
    } else {
        printf(" 0x%02X", state->A);
    }

    printf("   PC = 0x%04X   instruction = ", state->PC);

    printInstruction(state, state->PC);

    printf("\n");
}

static void executeListRegistersCommand(struct MachineState* state, char* command) {
    if (anyArgumentPresent(command)) return;

    listRegisters(state);
}

static void executeAddBreakpointCommand(char* command) {
    // TODO
}

static void executeDeleteBreakpointCommand(char* command) {
    // TODO
}

// Returns true if prompt interaction should continue, or false if simulation should resume
static bool executeContinueCommand(char* command) {
    if (anyArgumentPresent(command)) return true;

    return false;
}
// Returns true if prompt interaction should continue, or false if simulation should resume

static bool executeStepCommand(char* command) {
    if (anyArgumentPresent(command)) return true;

    isStepping = true;

    return false;
}

static void executeQuitCommand(char* command) {
    if (anyArgumentPresent(command)) return;

    printf("Quitting\n");
    exit(0);
}

// Returns true if prompt interaction should continue, or false if simulation should resume
static bool parseCommand(struct MachineState* state, char* command) {
    if (command[1] == 0 || isspace(command[1])) {
        switch (charUppercase(command[0]) ) {
            case 'H':
                executeHelpCommand(command);
                return true;
            case 'L':
                executeListMemoryCommand(state, command);
                return true;
            case 'A':
                executeListLabelsCommand(command);
                return true;
            case 'R':
                executeListRegistersCommand(state, command);
                return true;
            case 'B':
                executeAddBreakpointCommand(command);
                return true;
            case 'D':
                executeDeleteBreakpointCommand(command);
                return true;
            case 'C':
                return executeContinueCommand(command);
            case 'S':
                return executeStepCommand(command);
            case 'Q':
                executeQuitCommand(command);
                return true;
        }
    }

    printf("Unrecognized command. Type \"h\" to list all commands.\n");
    return true;
}

static void interactivePrompt(struct MachineState* state) {
    printf("Paused.   ");
    listRegisters(state);
    bool interactive = true;
    char command[128] = {0};
    while (interactive) {
        printf("> ");
        fgets(command, 127, stdin);
        interactive = parseCommand(state, command);
    }
}

void runDebug(struct MachineState* state, char* symbolsFilePath) {
    parseSymbolsFile(symbolsFilePath);

    printf("Starting in debug mode. Type \"h\" to list all commands or \"c\" to begin simulation. Press ^C during simulation to pause.\n");

    signal(SIGINT, handleSigInt);

    startCharacterInput();

    do {
        if (isPaused || isStepping) {
            isPaused = true;
            isStepping = false;
            endCharacterInput();
            interactivePrompt(state);
            startCharacterInput();
            if (isStepping) {
                step(state);
            }
            isPaused = false;
        } else {
            step(state);
        }
    } while (!state->isUnconditionalInfiniteLoop);

    endCharacterInput();

    printf("Unconditional infinite loop detected. Ending simulation.\n");
}
