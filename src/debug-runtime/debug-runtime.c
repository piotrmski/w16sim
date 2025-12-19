#include "debug-runtime.h"
#include "../machine-state/machine-state.h"
#include "../keyboard-input/keyboard-input.h"
#include "../time/time.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#define LABEL_NAME_MAX_LENGTH 31

enum DataType {
    DataTypeNone = 0,
    DataTypeInstruction,
    DataTypeChar,
    DataTypeInt
};

enum Command {
    CommandUnknown = 0,
    CommandHelp,
    CommandListMemory,
    CommandListBreakpoints,
    CommandListLabels,
    CommandListRegisters,
    CommandAddBreakpoint,
    CommandDeleteBreakpoint,
    CommandDeleteAllBreakpoints,
    CommandContinue,
    CommandStep,
    CommandQuit
};

struct Range {
    int start;
    int end;
};

static volatile bool isPaused = true;
static bool isStepping = false;
static char* labelNames[ADDRESS_SPACE_SIZE] = { NULL };
static bool breakpoints[ADDRESS_SPACE_SIZE] = { false };
static enum DataType dataTypes[ADDRESS_SPACE_SIZE] = { 
    DataTypeNone,
    [TIME_INTERFACE_ADDRESS] = DataTypeNone,
    [IO_INTERFACE_ADDRESS] = DataTypeChar 
};

static char charUppercase(char ch) {
    if (ch >= 'a' && ch <= 'z') return ch - 0x20;
    else return ch;
}

static bool stringsEqualCaseInsensitive(char* string1, char* string2) {
    for (int i = 0;; ++i) {
        if (charUppercase(string1[i]) != charUppercase(string2[i])) {
            return false;
        }

        if (string1[i] == 0 || string2[i] == 0) {
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
        if (stringsEqualCaseInsensitive(dataTypeString, "int")) {
            dataType = DataTypeInt;
        } else if (stringsEqualCaseInsensitive(dataTypeString, "char")) {
            dataType = DataTypeChar;
        } else if (stringsEqualCaseInsensitive(dataTypeString, "instruction")) {
            dataType = DataTypeInstruction;
        } else {
            printf("Error: in file \"%s\" line %d: unknown data type \"%s\".\n", path, lineNumber, dataTypeString);
            exit(1);
        }

        dataTypes[addressNumber] = dataType;

        if (labelName != NULL) {
            for (int i = 0; i < ADDRESS_SPACE_SIZE; ++i) {
                if (labelNames[i] != NULL && strcmp(labelName, labelNames[i]) == 0) {
                    printf("Error: in file \"%s\" line %d: label name \"%s\" is not unique.\n", path, lineNumber, labelName);
                    exit(1);
                }
            }

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

    for (int i = 0; i < ADDRESS_SPACE_SIZE - 1; ++i) {
        if (dataTypes[i] == DataTypeInstruction) {
            dataTypes[i+1] = DataTypeNone;
        }
    }
}

static void handleSigInt(int _) {
    if (isPaused) {
        printf("\nQuitting.\n");
        exit(0);
    } else {
        isPaused = true;
    }
}

static void printCharacterOrControlCharacter(unsigned char ch) {
    if (ch >= 32 && ch <= 126) {
        printf("'%c'", ch);
        return;
    }

    switch (ch) {
        case 0: printf("NUL"); break;
        case 1: printf("SOH"); break;
        case 2: printf("STX"); break;
        case 3: printf("ETX"); break;
        case 4: printf("EOT"); break;
        case 5: printf("ENQ"); break;
        case 6: printf("ACK"); break;
        case 7: printf("BEL"); break;
        case 8: printf("BS"); break;
        case 9: printf("TAB"); break;
        case 10: printf("LF"); break;
        case 11: printf("VT"); break;
        case 12: printf("FF"); break;
        case 13: printf("CR"); break;
        case 14: printf("SO"); break;
        case 15: printf("SI"); break;
        case 16: printf("DLE"); break;
        case 17: printf("DC1"); break;
        case 18: printf("DC2"); break;
        case 19: printf("DC3"); break;
        case 20: printf("DC4"); break;
        case 21: printf("NAK"); break;
        case 22: printf("SYN"); break;
        case 23: printf("ETB"); break;
        case 24: printf("CAN"); break;
        case 25: printf("EM"); break;
        case 26: printf("SUB"); break;
        case 27: printf("ESC"); break;
        case 28: printf("FS"); break;
        case 29: printf("GS"); break;
        case 30: printf("RS"); break;
        case 31: printf("US"); break;
        case 127: printf("DEL"); break;
        default: printf("0x%02X", ch);
    }
}

static void executeHelpCommand() {
    printf("Commands:\n\
h     - prints this message,\n\
l     - lists the memory value at PC,\n\
l X   - lists the memory value at X,\n\
l X:Y - lists memory values from X to Y,\n\
a     - lists all registered label names and their values,\n\
r     - lists register A value, program counter value, and instruction at PC,\n\
b     - adds a breakpoint at PC,\n\
b X   - adds a breakpoint at X,\n\
lb    - lists all breakpoints,\n\
d     - deletes a breakpoint at PC,\n\
d X   - deletes a breakpoint at X,\n\
da    - deletes all breakpoints,\n\
c     - continues simulation,\n\
s     - steps simulation (executes one instruction and pauses),\n\
q     - quits.\n\
Replace X and Y with one of the following:\n\
- a number - absolute address,\n\
- +C or -C where C is a number - address relative to program counter,\n\
- a label name,\n\
- L+C or L-C where L is a label name and C is a number - address relative to a label.\n");
}

static const char* getInstructionName(unsigned char opcode, bool pad) {
    switch (opcode) {
        case 0: return pad ? "LD " : "LD";
        case 1: return "NOT";
        case 2: return "ADD";
        case 3: return "AND";
        case 4: return pad ? "ST " : "ST";
        case 5: return "JMP";
        case 6: return "JMN";
        case 7: return "JMZ";
    }

    return "";
}

static void printInstruction(struct MachineState* state, int address, bool padInstructionName) {
    unsigned short instruction = peekInstruction(state, address);
    unsigned char opcode = instruction >> 13;
    unsigned short argument = instruction & 0x1fff;

    printf("%s ", getInstructionName(opcode, padInstructionName));

    if (labelNames[argument] == NULL) {
        printf("0x%04X", argument);
    } else {
        printf("%s", labelNames[argument]);
    }

    if (opcode < 4) {
        if (labelNames[argument] == NULL) {
            printf("    M[0x%04X] = ", argument);
        } else if (strlen(labelNames[argument]) > 8) {
            printf("    M[%c%c%c%c%c...] = ", labelNames[argument][0], labelNames[argument][1], labelNames[argument][2], labelNames[argument][3], labelNames[argument][4]);
        } else {
            printf("    M[%s] = ", labelNames[argument]);
        }

        unsigned char memoryValue = peekMemory(state, argument);

        if (dataTypes[argument] == DataTypeChar) {
            printCharacterOrControlCharacter(memoryValue);
        } else if (dataTypes[argument] == DataTypeInt) {
            printf("%d", memoryValue);
        } else {
            printf("0x%02X", memoryValue);
        }
    }
}

static int parseNumber(char* numberString, const char* numberDescription) {
    int offset = strtol(numberString, NULL, 0);
    if (errno != 0) {
        printf("\"%s\" is not a valid %s.\n", numberString, numberDescription);
    }
    return offset;
}

static int validateAddress(int address, char* addressExpression) {
    if (address < 0 || address >= ADDRESS_SPACE_SIZE) {
        printf("\"%s\" evaluates to 0x%04X which is not a valid address.\n", addressExpression, address);
        return -1;
    }
    return address;
}

static bool isFirstCharOfLabel(char ch) {
    return ch == '_' || ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z';
}

static int parseAddressArgument(struct MachineState* state, char* argument) {
    if (argument == NULL) {
        return state->PC;
    }

    if (argument[0] == '+' || argument[0] == '-') {
        int offset = parseNumber(argument, "offset");
        if (errno != 0) {
            errno = 0;
            return -1;
        }
        return validateAddress(state->PC + offset, argument);
    } else if (isFirstCharOfLabel(argument[0])) {
        int baseAddress = -1;
        int offset = 0;
        char* offsetString = strpbrk(argument, "+-");
        char offsetStringFirstChar;
        if (offsetString != NULL) {
            offsetStringFirstChar = offsetString[0];
            offset = parseNumber(offsetString, "offset");
            if (errno != 0) {
                errno = 0;
                return -1;
            }
            offsetString[0] = 0;
        }
        for (int i = 0; i < ADDRESS_SPACE_SIZE; ++i) {
            if (labelNames[i] != NULL && strcmp(argument, labelNames[i]) == 0) {
                baseAddress = i;
                break;
            }
        }
        if (baseAddress == -1) {
            printf("Label \"%s\" does not exist.\n", argument);
            return -1;
        }
        if (offsetString != NULL) {
            offsetString[0] = offsetStringFirstChar;
        }
        return validateAddress(baseAddress + offset, argument);
    } else {
        int address = parseNumber(argument, "address");
        if (errno != 0) {
            errno = 0;
            return -1;
        }
        return validateAddress(address, argument);
    }
    return 0;
}

static struct Range parseAddressRangeArgument(struct MachineState* state, char* argument) {
    if (argument == NULL) {
        return (struct Range) { state->PC, state->PC };
    }

    char* rangeDelimiter = strchr(argument, ':');

    if (rangeDelimiter == NULL) {
        int address = parseAddressArgument(state, argument);
        return (struct Range) { address, address };
    }

    char* extraRangeDelimiter = strchr(rangeDelimiter + 1, ':');

    if (extraRangeDelimiter != NULL || rangeDelimiter == argument || rangeDelimiter == (argument + strlen(argument) - 1)) {
        printf("\"%s\" is not a valid address range.\n", argument);
        return (struct Range) { -1, -1 };
    }

    *rangeDelimiter = 0;

    int start = parseAddressArgument(state, argument);
    if (start < 0) {
        return (struct Range) { -1, -1 };
    }

    int end = parseAddressArgument(state, rangeDelimiter + 1);
    if (end < 0) {
        return (struct Range) { -1, -1 };
    }

    if (end < start) {
        printf("Address range 0x%04X:0x%04X is invalid.\n", start, end);
        return (struct Range) { -1, -1 };
    }

    return (struct Range) { start, end };
}

static void printMemory(struct MachineState* state, unsigned short address, int maxLabelLength, bool printValueOfInstructionHigherBit) {
    bool labelDefined = labelNames[address] != NULL;

    printf(
        "%s %s 0x%04X %*s%s ",
        state->PC == address ? "PC" : "  ",
        breakpoints[address] ? "B" : " ",
        address,
        maxLabelLength,
        labelDefined ? labelNames[address] : "",
        labelDefined ? ":" : " "
    );

    unsigned char memVal = peekMemory(state, address);

    switch (dataTypes[address]) {
        case DataTypeNone:
            if (address > 0 && dataTypes[address - 1] == DataTypeInstruction) {
                if (printValueOfInstructionHigherBit || labelNames[address] != NULL) {
                    printf("0x%02X (second byte of a %s instruction)", memVal, getInstructionName(memVal >> 5, false));
                }
            } else {
                printf("0x%02X", memVal);
            }
            break;
        case DataTypeInstruction:
            printInstruction(state, address, true);
            break;
        case DataTypeChar:
            printCharacterOrControlCharacter(memVal);
            break;
        case DataTypeInt:
            printf("%d", memVal);
            break;
    }

    printf("\n");
}

static void executeListMemoryCommand(struct MachineState* state, char* argument) {
    struct Range addresses = parseAddressRangeArgument(state, argument);
    if (addresses.start < 0) return;

    int longestLabelNameLength = 0;
    for (int i = addresses.start; i <= addresses.end; ++i) {
        int labelNameLength = labelNames[i] != NULL ? strlen(labelNames[i]) : 0;
        longestLabelNameLength = longestLabelNameLength > labelNameLength ? longestLabelNameLength : labelNameLength;
    }

    for (int i = addresses.start; i <= addresses.end; ++i) {
        printMemory(state, i, longestLabelNameLength, i == addresses.start);
    }
}

static void executeListBreakpointsCommand(struct MachineState* state) {
    int longestLabelNameLength = 0;
    bool anyBreakpointDefined = false;
    for (int i = 0; i < ADDRESS_SPACE_SIZE; ++i) {
        if (breakpoints[i]) {
            anyBreakpointDefined = true;
            int labelNameLength = labelNames[i] != NULL ? strlen(labelNames[i]) : 0;
            longestLabelNameLength = longestLabelNameLength > labelNameLength ? longestLabelNameLength : labelNameLength;
        }
    }

    if (!anyBreakpointDefined) {
        printf("No breakpoints added.\n");
        return;
    }

    for (int i = 0; i < ADDRESS_SPACE_SIZE; ++i) {
        if (breakpoints[i]) {
            printMemory(state, i, longestLabelNameLength, true);
        }
    }
    if (!anyBreakpointDefined) {
        printf("No breakpoints added.\n");
    }
}

static void executeListLabelsCommand() {
    bool anyLabelDefined = false;
    for (int i = 0; i < ADDRESS_SPACE_SIZE; ++i) {
        if (labelNames[i] != NULL) {
            printf("0x%04X %s\n", i, labelNames[i]);
            anyLabelDefined = true;
        }
    }
    if (!anyLabelDefined) {
        printf("No labels defined.\n");
    }
}

static void executeListRegistersCommand(struct MachineState* state) {
    printf("A = 0x%02X %d", state->A, state->A);

    if (state->A <= 127) {
        putchar(' ');
        printCharacterOrControlCharacter(state->A);
    }

    printf("    PC = 0x%04X", state->PC);
    if (labelNames[state->PC] != NULL) {
        printf(" %s", labelNames[state->PC]);
    }

    printf("    instruction = ");

    printInstruction(state, state->PC, false);

    printf("\n");
}

static void executeAddBreakpointCommand(struct MachineState* state, char* argument) {
    int address = parseAddressArgument(state, argument);
    if (address < 0) return;
    
    if (breakpoints[address]) {
        printf("Breakpoint at 0x%04X was already added.\n", address);
    } else {
        breakpoints[address] = true;
        printf("Added a breakpoint at 0x%04X.\n", address);
    }
}

static void executeDeleteBreakpointCommand(struct MachineState* state, char* argument) {
    int address = parseAddressArgument(state, argument);
    if (address < 0) return;
    
    if (breakpoints[address]) {
        breakpoints[address] = false;
        printf("Deleted a breakpoint at 0x%04X.\n", address);
    } else {
        printf("There isn't a breakpoint at 0x%04X.\n", address);
    }
}

static void executeDeleteAllBreakpointsCommand() {
    int breakpointsDeleted = 0;
    for (int i = 0; i < ADDRESS_SPACE_SIZE; ++i) {
        if (breakpoints[i]) {
            ++breakpointsDeleted;
            breakpoints[i] = false;
        }
    }

    if (breakpointsDeleted == 0) {
        printf("There aren't any breakpoints.\n");
    } else {
        printf("%d breakpoints deleted.\n", breakpointsDeleted);
    }
}

static enum Command getCommand(char* commandName) {
    if (stringsEqualCaseInsensitive(commandName, "H")) {
        return CommandHelp;
    } else if (stringsEqualCaseInsensitive(commandName, "L")) {
        return CommandListMemory;
    } else if (stringsEqualCaseInsensitive(commandName, "LB")) {
        return CommandListBreakpoints;
    } else if (stringsEqualCaseInsensitive(commandName, "A")) {
        return CommandListLabels;
    } else if (stringsEqualCaseInsensitive(commandName, "R")) {
        return CommandListRegisters;
    } else if (stringsEqualCaseInsensitive(commandName, "B")) {
        return CommandAddBreakpoint;
    } else if (stringsEqualCaseInsensitive(commandName, "D")) {
        return CommandDeleteBreakpoint;
    } else if (stringsEqualCaseInsensitive(commandName, "DA")) {
        return CommandDeleteAllBreakpoints;
    } else if (stringsEqualCaseInsensitive(commandName, "C")) {
        return CommandContinue;
    } else if (stringsEqualCaseInsensitive(commandName, "S")) {
        return CommandStep;
    } else if (stringsEqualCaseInsensitive(commandName, "Q")) {
        return CommandQuit;
    } else {
        return CommandUnknown;
    }
}

// Returns true if prompt interaction should continue, or false if simulation should resume
static bool executeCommand(struct MachineState* state, char* fullCommand) {
    char* commandName = strtok(fullCommand, " \n");
    char* argument = strtok(NULL, " \n");
    char* extra = strtok(NULL, " \n");

    if (commandName == NULL) {
        return true;
    }

    enum Command command = getCommand(commandName);

    if (command == CommandUnknown) {
        printf("Unrecognized command \"%s\". Type \"h\" to list all commands.\n", commandName);
        return true;
    }

    switch (command) {
        case CommandListMemory:
        case CommandAddBreakpoint:
        case CommandDeleteBreakpoint:
            if (extra != NULL) {
                printf("Command \"%s\" doesn't take more than one argument. Type \"h\" to list all commands.\n", commandName);
                return true;
            }
            break;
        default:
            if (argument != NULL) {
                printf("Command \"%s\" doesn't take any arguments. Type \"h\" to list all commands.\n", commandName);
                return true;
            }
            break;
    }

    switch (command) {
        case CommandHelp:
            executeHelpCommand(); break;
        case CommandListMemory:
            executeListMemoryCommand(state, argument); break;
        case CommandListBreakpoints:
            executeListBreakpointsCommand(state); break;
        case CommandListLabels:
            executeListLabelsCommand(); break;
        case CommandListRegisters:
            executeListRegistersCommand(state); break; 
        case CommandAddBreakpoint:
            executeAddBreakpointCommand(state, argument); break;
        case CommandDeleteBreakpoint:
            executeDeleteBreakpointCommand(state, argument); break;
        case CommandDeleteAllBreakpoints:
            executeDeleteAllBreakpointsCommand(); break;
        case CommandContinue:
            return false;
        case CommandStep:
            isStepping = true;
            return false;
        case CommandQuit:
            printf("Quitting.\n");
            exit(0);
        default: break;
    }

    return true;
}

static void interactivePrompt(struct MachineState* state) {
    printf("Paused.   ");
    executeListRegistersCommand(state);
    bool interactive = true;
    char fullCommand[128] = {0};
    while (interactive) {
        printf("> ");
        fgets(fullCommand, 127, stdin);
        interactive = executeCommand(state, fullCommand);
    }
}

void runDebug(struct MachineState* state, char* symbolsFilePath) {
    parseSymbolsFile(symbolsFilePath);

    printf("Starting in debug mode. Type \"h\" to list all commands or \"c\" to begin simulation. Press ^C during simulation to pause.\n");

    signal(SIGINT, handleSigInt);

    startCharacterInput();

    do {
        if (isPaused || isStepping || breakpoints[state->PC]) {
            isPaused = true;
            isStepping = false;
            unsigned long idleStartTime = getTimeMs();
            endCharacterInput();
            interactivePrompt(state);
            startCharacterInput();
            state->simulationIdleTimeMs += getTimeMs() - idleStartTime;
            isPaused = false;
        }
        
        step(state);
    } while (!state->isUnconditionalInfiniteLoop);

    endCharacterInput();

    printf("Unconditional infinite loop detected. Ending simulation.\n");
}
