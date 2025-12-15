#include "default-runtime.h"
#include "../machine-state/machine-state.h"
#include "../keyboard-input/keyboard-input.h"

void runDefault(struct MachineState* state) {
    startCharacterInput();

    do {
        step(state);
    } while (!state->isUnconditionalInfiniteLoop);

    endCharacterInput();
}
