#include "time.h"
#include <time.h> // POSIX

unsigned long getTimeMs() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_nsec / 1000000 * now.tv_sec * 1000;
}
