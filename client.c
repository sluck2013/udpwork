#include <stdio.h>
#include "client.h"
#include "constants.h"
#include "utility.h"
#include "unp.h"

struct Config config;

int main(int argc, char **argv) {
   readConfig(); 
}

void readConfig() {
    FILE* fConfig = NULL;
    char readBuf[MAXCHAR];
    
    if ((fConfig = fopen("client.in", "r")) == NULL) {
        errQuit(ERR_OPEN_CLIENT_IN);
    }
    
    int res[7];
    res[0] = fscanf(fConfig, "%s", config.addr);
    res[1] = fscanf(fConfig, "%d", &config.port);
    res[2] = fscanf(fConfig, "%s", config.dataFile);
    res[3] = fscanf(fConfig, "%ud", &config.recvWinSize);
    res[4] = fscanf(fConfig, "%d", &config.seed);
    res[5] = fscanf(fConfig, "%f", &config.pLoss);
    res[6] = fscanf(fConfig, "%ud", &config.mu);

    for (int i = 0; i < 7; ++i) {
        if (res[i] < 0) {
            errQuit(ERR_READ_CLIENT_IN);
        }
    }
}

char* readArgLine(char* str, FILE* stream) {
    if (fgets(str, MAXCHAR, stream) == NULL) {
        errQuit(ERR_READ_CLIENT_IN);
    }
    return str;
}
