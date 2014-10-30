#ifndef CLIENT_H
#define CLIENT_H

#include "constants.h"
#include "lib/unpifiplus.h"

struct Config {
    // read from client.in
    char serverAddr[MAXCHAR];
    int port;
    char dataFile[MAXCHAR];
    unsigned int recvWinSize;
    int seed;
    float pLoss;
    unsigned int mu;

    // set by IP checking
    char IPServer[MAXCHAR];
    char IPClient[MAXCHAR];
};

void readConfig();
int isOnSameHost(struct ifi_info *ifiHead);
int isLocal(struct ifi_info* ifiHead, struct ifi_info **ifiMatch);
int getPrefixLen(uint32_t a, uint32_t b);

#endif
