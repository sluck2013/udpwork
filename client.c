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

    /* set IPClient */
    struct ifi_info *ifiMatch = NULL;
    if (isOnSameHost(ifiHead)) {
        strcpy(config.IPClient, "127.0.0.1");
        strcpy(config.IPServer, "127.0.0.1");
    } else if (isLocal(ifiHead, &ifiMatch)) {
        struct sockaddr *sa = ifiMatch->ifi_addr;
        strcpy(config.IPClient, Sock_ntop_host(sa, sizeof(*sa)));
    } else {
        struct sockaddr *sa = ifiHead->ifi_addr;
        strcpy(config.IPClient, Sock_ntop_host(sa, sizeof(*sa)));
    }

    free_ifi_info_plus(ifiHead);

    int sockfd;
    struct sockaddr_in siMyAddr, siGotAddr;
    socklen_t slAddrLen = sizeof(siGotAddr);
    struct in_addr iaGotAddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&siMyAddr, sizeof(siMyAddr));
    siMyAddr.sin_family = AF_INET;
    inet_pton(AF_INET, config.IPClient, &siMyAddr.sin_addr);
    siMyAddr.sin_port = htons(0);
    
    Bind(sockfd, (SA*)&siMyAddr, sizeof(siMyAddr));
    getsockname(sockfd, (SA*)&siGotAddr, &slAddrLen);
    printf("%u\n", ntohs(siGotAddr.sin_port));
    printf("%s\n", Sock_ntop_host((SA*)&siGotAddr, sizeof(siGotAddr)));

    //struct sockaddr *sa = &saMyaddr;
    //printf("%s\n", Sock_ntop_host(sa, sizeof(*sa)));
    //printf("%s\n", &saMyaddr);
}

/*
 * read information from client.in
 */
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
 * @return int 1 if same, 0 if different
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

/*
 * check if server and client are local to each other.
 * If local, return ifi pointer, to where IP address is local
 * to IPServer, using "longest prefix matching" strategy.
 * @return int 1 if is local, 0 otherwise
 * @param struct ifi_info *ifiHead pointer to head of linked
 *        list returned by Get_ifi_info_plus()
 * @param struct ifi_info **ifiMatch pointer to pointer of
 *        ifi_info struct, the selected pointer to ifi_info
 *        struct whose IP is local to IPServer will be stored
 *        in *ifiMatch
 */
int isLocal(struct ifi_info *ifiHead, struct ifi_info **ifiMatch) {
    struct in_addr clientAddr, serverAddr, maskAddr;
    inet_pton(AF_INET, config.serverAddr, &serverAddr);
    int iIsLocal = 0;
    int iMaxPrefixLen = 0;

    for (struct ifi_info *ifi = ifiHead; ifi != NULL; ifi = ifi->ifi_next) {
        clientAddr = ((struct sockaddr_in*)ifi->ifi_addr)->sin_addr;
        maskAddr = ((struct sockaddr_in*)ifi->ifi_ntmaddr)->sin_addr;
        uint32_t r1 = serverAddr.s_addr & maskAddr.s_addr;
        uint32_t r2 = clientAddr.s_addr & maskAddr.s_addr;
        if (r1 == r2) {
            int iPrefixLen = getPrefixLen(serverAddr.s_addr, clientAddr.s_addr);
            if (iPrefixLen <= iMaxPrefixLen) {
                continue;
            }
            iMaxPrefixLen = iPrefixLen;
            iIsLocal = 1;
            (*ifiMatch) = ifi;
        }
    }

    return iIsLocal;
}

/* 
 * compute length of prefix of two IP addresses.
 * @return int length of prefix
 * @param uint32_t a, b two IP addresses to be compared
 */
int getPrefixLen(uint32_t a, uint32_t b) {
    int iCnt = 0;
    while (a != b) {
        a <<= 1;
        b <<= 1;
        ++iCnt;
    }
    return iCnt;
}
