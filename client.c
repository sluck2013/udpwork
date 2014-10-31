#include <stdio.h>
#include "client.h"
#include "constants.h"
#include "utility.h"
#include "unp.h"
#include "lib/get_ifi_info_plus.c"

struct Config config;

int main(int argc, char **argv) {
    readConfig();      // read client.in
    setIPClient();     // setIPClient
    createUDPSocket(); // create UDP socket
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
    strcpy(config.IPServer, config.serverAddr);
    fclose(fConfig);
}

/* 
 * retrieve and print interface infomation, set IPClient according
 * to IPServer
 */
void setIPClient() {
    /* retrieve & print interface info */
    struct ifi_info *ifi = Get_ifi_info_plus(AF_INET, 1);
    struct ifi_info *ifiHead = ifi;

    for (; ifi != NULL; ifi = ifi->ifi_next) {
        printIfiInfo(ifi);
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
    printItem("IPClient is set to", config.IPClient);
    printItem("IPServer is set to", config.IPServer);
    println();
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
inline int getPrefixLen(uint32_t a, uint32_t b) {
    int iCnt = 0;
    while (a != b) {
        a <<= 1;
        b <<= 1;
        ++iCnt;
    }
    return iCnt;
}

/*
 * create UDP Socket
 */
void createUDPSocket() {
    int sockfd;
    //siClientAddr - designated by IPClient
    //siServerADdr - designated by IPServer
    //siLocalAddr - returned by getsockname()
    //siForeignAddr - returned by getpeername()
    struct sockaddr_in siClientAddr, siServerAddr, siLocalAddr, siForeignAddr;
    socklen_t slLocalLen = sizeof(siLocalAddr);
    socklen_t slForeignLen = sizeof(siForeignAddr);
    struct in_addr iaLocalAddr;


    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    printInfo("UDP socket created");

    bzero(&siClientAddr, sizeof(siClientAddr));
    siClientAddr.sin_family = AF_INET;
    inet_pton(AF_INET, config.IPClient, &siClientAddr.sin_addr);
    siClientAddr.sin_port = htons(0);
    
    bind(sockfd, (SA*)&siClientAddr, sizeof(siClientAddr));
    printInfo("Port binded");

    getsockname(sockfd, (SA*)&siLocalAddr, &slLocalLen);
    printSockInfo(&siLocalAddr, "Local");

    bzero(&siServerAddr, sizeof(siServerAddr));
    siServerAddr.sin_family = AF_INET;
    inet_pton(AF_INET, config.IPServer, &siServerAddr.sin_addr);
    siServerAddr.sin_port = htons(config.port);
    connect(sockfd, (SA*)&siServerAddr, sizeof(siServerAddr));
    printInfo("Connected to server");

    getpeername(sockfd, (SA*)&siForeignAddr, &slForeignLen);
    printSockInfo(&siForeignAddr, "Foreign");
    Write(sockfd, config.dataFile, strlen(config.dataFile));
}

/*
 * Print IP address and port number from sockaddr_in struct.
 * Used to print info returned by getsockname and getpeername.
 * @param struct sockaddr_in* addr pointer to sockaddr_in variable
 *        containing IP server and port number
 * @param char* addrName string which should be either "Local"
 *        or "Foreign"
 */
void printSockInfo(struct sockaddr_in* addr, char* addrName) {
    unsigned int uiPort = ntohs(addr->sin_port);
    char* pcIP = Sock_ntop_host((SA*)addr, sizeof(*addr));
    printf("%s address assigned to socket: %s:%u\n", addrName, pcIP, uiPort);
}
