#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include "lib/unpifiplus.h"

void printErr(char *errMsg);
void errQuit(char *errMsg);
void printItem(const char* key, const char* value);
void println();
void printInfo(char *info);
void printIfiInfo(struct ifi_info* ifi);
void printSockInfo(struct sockaddr_in* addr, char* addrName);

#endif
