#ifndef CLIENT_H
#define CLIENT_H

#include "constants.h"
#include "lib/unpifiplus.h"

struct Config {
    // read from client.in
    char serverAddr[MAXCHAR];
    int port;
    char dataFile[MAXCHAR];
    unsigned short int recvWinSize;
    int seed;
    float pLoss;
    unsigned int mu;

    // set by IP checking
    char IPServer[MAXCHAR];
    char IPClient[MAXCHAR];
};

// used to thread creation
struct Arg {
    int sockfd;
};

void readConfig();
void setIPClient();
void createUDPSocket();
int isOnSameHost(struct ifi_info *ifiHead);
int isLocal(struct ifi_info* ifiHead, struct ifi_info **ifiMatch);
inline int getPrefixLen(uint32_t a, uint32_t b);
inline unsigned short int getWindowSize();
void* printData(void *arg);
void getSleepTime(struct timespec* tm);
static void sig_alrm(int signo);
void sig_chld(int signo);
int isDropped();

#endif
