#include <stdio.h>
#include "client.h"
#include "constants.h"
#include "utility.h"
#include "unp.h"
#include "lib/get_ifi_info_plus.c"

struct Config config;

int main(int argc, char **argv) {
    // read client.in
    readConfig();

    /* retrieve & print interface info */
    struct ifi_info *ifi = Get_ifi_info_plus(AF_INET, 1);
    struct ifi_info *ifiHead = ifi;

    for (; ifi != NULL; ifi = ifi->ifi_next) {
        struct sockaddr *sa = ifi->ifi_addr;
        printItem("IP address", Sock_ntop_host(sa, sizeof(*sa)));
        sa = ifi->ifi_ntmaddr;
        printItem("Network mask", Sock_ntop_host(sa, sizeof(*sa)));
        println();       
    }

    if (isOnSameHost(ifiHead)) {
        strcpy(config.clientAddr, "127.0.0.1");
    } else {
    }
    free_ifi_info_plus(ifiHead);


}

void readConfig() {
    FILE* fConfig = NULL;
    
    if ((fConfig = fopen("client.in", "r")) == NULL) {
        errQuit(ERR_OPEN_CLIENT_IN);
    }
    
    int res[7];
    res[0] = fscanf(fConfig, "%s", config.serverAddr);
    res[1] = fscanf(fConfig, "%d", &config.port);
    res[2] = fscanf(fConfig, "%s", config.dataFile);
    res[3] = fscanf(fConfig, "%ud", &config.recvWinSize);
    res[4] = fscanf(fConfig, "%d", &config.seed);
    res[5] = fscanf(fConfig, "%f", &config.pLoss);
    res[6] = fscanf(fConfig, "%ud", &config.mu);

    for (int i = 0; i < 7; ++i) {
        if (res[i] <= 0) {
            errQuit(ERR_READ_CLIENT_IN);
        }
    }
}

/*
 * check if server and client are on the same host
 * @return 1 if same, 0 if different
 * @param const struct ifi_info *ifHead pointer to head of
 *        linked list returned by Get_ifi_info_plus()
 */
int isOnSameHost(struct ifi_info *ifiHead) {
    for (struct ifi_info *ifi = ifiHead; ifi != NULL; ifi = ifi->ifi_next) {
        struct sockaddr *sa = ifi->ifi_addr;
        char *clientIP = Sock_ntop_host(sa, sizeof(*sa));
        if (strcmp(config.serverAddr, clientIP) == 0) {
            return 1;
        }
    }
    return 0;
}

