#ifndef CLIENT_H
#define CLIENT_H

#include "constants.h"

struct Config {
    char addr[MAXCHAR];
    int port;
    char dataFile[MAXCHAR];
    unsigned int recvWinSize;
    int seed;
    float pLoss;
    unsigned int mu;
};

void readConfig();

#endif
