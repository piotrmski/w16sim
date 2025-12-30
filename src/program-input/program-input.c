#include "program-input.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

struct ProgramInput getProgramInput(int argc, const char * argv[]) {
    const char* binaryFilePath = NULL;
    const char* symbolsFilePath = NULL;
    int clockFrequencyKiloHz = 1;

    bool helpFlag = false;
    bool symbolsFlag = false;
    bool debugFlag = false;
    bool clockFlag = false;

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                if (helpFlag) {
                    printf("Error: help flag was used more than once.\n");
                    exit(1);
                } else {
                    helpFlag = true;
                }
            } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--symbols") == 0) {
                if (symbolsFlag) {
                    printf("Error: symbols flag was used more than once.\n");
                    exit(1);
                } else if (i == argc - 1) {
                    printf("Error: symbols file path was not provided.\n");
                    exit(1);
                } else {
                    symbolsFilePath = argv[++i];
                    symbolsFlag = true;
                }
            } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
                if (debugFlag) {
                    printf("Error: debug flag was used more than once.\n");
                    exit(1);
                } else {
                    debugFlag = true;
                }
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--clock") == 0) {
                if (clockFlag) {
                    printf("Error: clock flag was used more than once.\n");
                    exit(1);
                } else if (i == argc - 1) {
                    printf("Error: clock frequency was not provided.\n");
                    exit(1);
                } else {
                    clockFrequencyKiloHz = strtol(argv[++i], NULL, 0);
                    if (errno != 0 || clockFrequencyKiloHz < 1 || clockFrequencyKiloHz > 1000) {
                        printf("Error: \"%s\" is not a valid clock frequency.\n", argv[i]);
                        exit(1);
                    }
                    clockFlag = true;
                }
            } else {
                printf("Error: unknown flag \"%s\".\n", argv[i]);
                exit(1);
            }
        } else {
            if (binaryFilePath != NULL) {
                printf("Error: binary file path was provided more than once.\n");
                exit(1);
            } else {
                binaryFilePath = argv[i];
            }
        }
    }

    if (argc == 1 || helpFlag) {
        printf("W13 simulator. Copyright (C) 2025 Piotr Marczyński. This program is licensed under GNU GPL v3. See file COPYING.\n");
        printf("Usage:\n");
        printf("w13sim [path/to/binary.bin]\n");
        printf("runs the simulator until ^C is pressed, or until a JMP instruction to the current address (unconditional infinite loop) is detected.\n\n");
        printf("Options:\n");
        printf("-c [frequency] or --clock [frequency] - sets maximum clock frequency in kHz. Must be between 1 and 1000000. Default is 1000.\n");
        printf("-h or --help - prints this message.\n");
        printf("-d or --debug - runs the simulator in paused state and enables the debugger.\n");
        printf("-s [path/to/symbols.csv] or --symbols [path/to/symbols.csv] - supplies the debugger with symbols info. Without -d or --debug it is ignored.\n\n");
        printf("The symbols file must be in CSV format with three columns:\n");
        printf("- the memory address,\n");
        printf("- data type (one of following: \"char\", \"int\", or \"instruction\"),\n");
        printf("- label name (unique; 0-31 characters: digits, upper- or lowercase letters, and underscores; the first character can't be a digit).\n");
        exit(0);
    } else if (binaryFilePath == NULL) {
        printf("Error: binary file path was not provided.\n");
        exit(1);
    }

    return (struct ProgramInput) { debugFlag, binaryFilePath, symbolsFilePath, clockFrequencyKiloHz };
}