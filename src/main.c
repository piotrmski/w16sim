/*
    W16SIM Copyright (C) 2025 Piotr Marczyński <piotrmski@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    See file COPYING.
*/

#include <stdio.h>
#include <stdbool.h>
#include "program-input/program-input.h"
#include "machine-state/machine-state.h"
#include "debug-runtime/debug-runtime.h"
#include "default-runtime/default-runtime.h"

int main(int argc, const char * argv[]) {
    struct ProgramInput input = getProgramInput(argc, argv);

    if (input.error) { return 1; }

    if (input.binaryFilePath == NULL) { return 0; }

    FILE* binaryFile = fopen(input.binaryFilePath, "rb");

    if (binaryFile == NULL) {
        printf("Error: could not read file \"%s\"\n.", input.binaryFilePath);
        return 1;
    }

    fseek(binaryFile, 0, SEEK_END);
    int fileSize = ftell(binaryFile);
    int programLength = fileSize;

    if (programLength > 0x1FFF) {
        printf("Error: The binary file size is invalid, should be less than 8192 bytes.\n");
        fclose(binaryFile);
        return 1;
    }

    struct MachineState state = getInitialState();

    fseek(binaryFile, 0, SEEK_SET);
    fread(state.memory, sizeof(unsigned char), programLength, binaryFile);
    fclose(binaryFile);

    if (input.debugMode) {
        runDebug(&state, input.symbolsFilePath);
    } else {
        runDefault(&state);
    }

    return 0;
}