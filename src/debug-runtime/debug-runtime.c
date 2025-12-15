#include "debug-runtime.h"
#include "../machine-state/machine-state.h"
#include "../keyboard-input/keyboard-input.h"

void runDebug(struct MachineState* state, const char* symbolsFilePath) {
    // TODO check symbols file existence and validity, and create the debug runtime

    startCharacterInput();

    do {
        step(state);
    } while (!state->isUnconditionalInfiniteLoop);

    endCharacterInput();
}
