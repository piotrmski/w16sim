#include "default-runtime.h"
#include "../machine-state/machine-state.h"
#include "../keyboard-input/keyboard-input.h"
#include <unistd.h> // POSIX

void runDefault(struct MachineState* state) {
    startCharacterInput();

    do {
        step(state);
        usleep(100); // TODO configurable clock
    } while (!state->isUnconditionalInfiniteLoop);

    endCharacterInput();
}
