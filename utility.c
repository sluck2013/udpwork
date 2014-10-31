//#include <stdio.h>
#include <stdlib.h>
#include "utility.h"

void printErr(char *errMsg) {
    fprintf(stderr, "ERROR: %s\n", errMsg);
}

void errQuit(char *errMsg) {
    printErr(errMsg);
    exit(0);
}

void printItem(const char* key, const char* value) {
    printf("%s: %s\n", key, value);
}

void println() {
    printf("\n");
}

void printInfo(char* info) {
    printItem("INFO", info);
}
