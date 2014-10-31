#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>

void printErr(char *errMsg);
void errQuit(char *errMsg);
void printItem(const char* key, const char* value);
void println();
void printInfo(char *info);

#endif
