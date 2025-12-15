#include "program-input.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

struct ProgramInput getProgramInput(int argc, const char * argv[]) {
    struct ProgramInput errorResult = {
        true,
        false,
        NULL,
        NULL
    };

    const char* binaryFilePath = NULL;
    const char* symbolsFilePath = NULL;

    bool helpFlag = false;
    bool symbolsFlag = false;
    bool debugFlag = false;

    bool expectNextArgToBeSymbolsFilePath = false;

    for (int i = 1; i < argc; ++i) {
        if (expectNextArgToBeSymbolsFilePath) {
            symbolsFilePath = argv[i];
            expectNextArgToBeSymbolsFilePath = false;
        } else if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                if (helpFlag) {
                    printf("Error: help flag was used more than once.\n");
                    return errorResult;
                } else {
                    helpFlag = true;
                }
            } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--symbols") == 0) {
                if (symbolsFlag) {
                    printf("Error: symbols flag was used more than once.\n");
                    return errorResult;
                } else {
                    symbolsFlag = true;
                    expectNextArgToBeSymbolsFilePath = true;
                }
            } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
                if (debugFlag) {
                    printf("Error: debug flag was used more than once.\n");
                    return errorResult;
                } else {
                    debugFlag = true;
                }
            } else {
                printf("Error: unknown flag \"%s\".\n", argv[i]);
                return errorResult;
            }
        } else {
            if (binaryFilePath != NULL) {
                printf("Error: binary file path was provided more than once.\n");
            } else {
                binaryFilePath = argv[i];
            }
        }
    }

    if (argc == 1 || helpFlag) {
        printf("W16 simulator. Copyright (C) 2025 Piotr Marczyński. This program is licensed under GNU GPL v3. See file COPYING.\n");
        printf("Usage:\n");
        printf("w16sim [path/to/binary.bin]\n");
        printf("runs the simulator until ^C is pressed, or until a JMP instruction to the current address (unconditional infinite loop) is detected.\n\n");
        printf("Flags:\n");
        printf("-h or --help - prints this message.\n");
        printf("-d or --debug - runs the simulator in paused state and enables the debugger.\n");
        printf("-s [path/to/symbols.csv] or --symbols [path/to/symbols.csv] - supplies the debugger with symbols info. Without -d or --debug it is ignored.\n\n");
        printf("The symbols file must be in CSV format with three columns:\n");
        printf("- the memory address in 4-digit hexadecimal format (0x0000 - 0x1FFF; x lowercase, A-F uppercase),\n");
        printf("- data type (one of following: \"char\", \"int\", or \"instruction\", all lowercase),\n");
        printf("- label name (0-31 characters: digits, upper- or lowercase letters, and underscores; the first character can't be a digit).\n");
        return (struct ProgramInput) { false, false, NULL, NULL };
    } else if (expectNextArgToBeSymbolsFilePath) {
        printf("Error: labels file path was not provided.\n");
        return errorResult;
    } else if (binaryFilePath == NULL) {
        printf("Error: binary file path was not provided.\n");
        return errorResult;
    }

    return (struct ProgramInput) { false, debugFlag, binaryFilePath, symbolsFilePath };
}