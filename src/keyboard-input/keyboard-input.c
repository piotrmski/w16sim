#include "keyboard-input.h"
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h> // POSIX
#include <termios.h> // POSIX
#include <unistd.h> // POSIX

static pthread_t thread;
static pthread_mutex_t lock;
static char ch = 0;
static bool active = false;

static void* readChar(void* _) {
    int readChar = 0;

    do {
        readChar = getchar();

        if (readChar < 0) {
            clearerr(stdin);
        } else {
            pthread_mutex_lock(&lock);
            ch = readChar;
            pthread_mutex_unlock(&lock);
        }
    } while (active);

    return NULL;
}

void startCharacterInput() {
    struct termios attr;

    tcgetattr(STDIN_FILENO, &attr);
    attr.c_lflag &= ~ICANON;
    attr.c_lflag &= ~ECHO;
    attr.c_cc[VMIN] = 1;
    attr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &attr);

    active = true;

    pthread_create(&thread, NULL, readChar, NULL);
}

void endCharacterInput() {
    active = false;

    struct termios attr;
    
    tcgetattr(STDIN_FILENO, &attr);
    attr.c_cc[VMIN] = 0;
    attr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &attr);

    pthread_join(thread, NULL);

    tcgetattr(STDIN_FILENO, &attr);
    attr.c_lflag |= ICANON;
    attr.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &attr);
}

char getLastChar() {
    pthread_mutex_lock(&lock);
    char result = ch;
    ch = 0;
    pthread_mutex_unlock(&lock);
    
    return result;
}

char peekLastChar() {
    pthread_mutex_lock(&lock);
    char result = ch;
    pthread_mutex_unlock(&lock);

    return result;
}