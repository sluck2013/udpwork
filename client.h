#ifndef CLIENT_H
#define CLIENT_H

#include "constants.h"
#include "lib/unpifiplus.h"

struct Config {
    char serverAddr[MAXCHAR];
    char clientAddr[MAXCHAR];
    int port;
    char dataFile[MAXCHAR];
    unsigned int recvWinSize;
    int seed;
    float pLoss;
    unsigned int mu;
};

void readConfig();
int isOnSameHost(struct ifi_info *ifiHead);

#endif
