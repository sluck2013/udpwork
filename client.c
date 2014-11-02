#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "client.h"
#include "constants.h"
#include "utility.h"
#include "unp.h"
#include "lib/get_ifi_info_plus.c"
#include "unpthread.h"
#include "common.h"


struct Config config;
struct Payload plReadBuf[MAX_BUF_SIZE];
unsigned long int iBufBase = 0;
unsigned long int iBufEnd = 0;
unsigned long int seqNum = 10;

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


    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    printInfo("UDP socket created");

    bzero(&siClientAddr, sizeof(siClientAddr));
    siClientAddr.sin_family = AF_INET;
    Inet_pton(AF_INET, config.IPClient, &siClientAddr.sin_addr);
    siClientAddr.sin_port = htons(0);
    
    bind(sockfd, (SA*)&siClientAddr, sizeof(siClientAddr));
    printInfo("Port binded");

    Getsockname(sockfd, (SA*)&siLocalAddr, &slLocalLen);
    printSockInfo(&siLocalAddr, "Local");

    bzero(&siServerAddr, sizeof(siServerAddr));
    siServerAddr.sin_family = AF_INET;
    Inet_pton(AF_INET, config.IPServer, &siServerAddr.sin_addr);
    siServerAddr.sin_port = htons(config.port);
    Connect(sockfd, (SA*)&siServerAddr, sizeof(siServerAddr));
    printInfo("Connected to server"); // TODO: add addr.

    Getpeername(sockfd, (SA*)&siForeignAddr, &slForeignLen);
    printSockInfo(&siForeignAddr, "Foreign");
    Write(sockfd, config.dataFile, strlen(config.dataFile));

    // get server's "connection" socket port
    struct Payload rawNewPort;
    Read(sockfd, &rawNewPort, sizeof(rawNewPort));
    config.port = atoi(rawNewPort.data);

    //reconnect to "connection" socket
    siServerAddr.sin_port = htons(config.port);
    Connect(sockfd, (SA*)&siServerAddr, sizeof(siServerAddr));
    char msg[MAXLINE];
    sprintf(msg, "Reconnected to server at port %d", config.port);
    printInfo(msg);

    //send back ack
    struct Payload newPortAck;
    newAck(&newPortAck, seqNum++, rawNewPort.header.seqNum, min(
               MAX_BUF_SIZE, config.recvWinSize
               ), rawNewPort.header.timestamp
          );
    Write(sockfd, &newPortAck, sizeof(newPortAck));

    pthread_t tid;
    struct Arg arg;
    arg.sockfd = sockfd;

    Pthread_create(&tid, NULL, printData, NULL);
    while (1) {
        Read(sockfd, &plReadBuf[iBufEnd], sizeof(plReadBuf[iBufEnd]));
        printf("%s", plReadBuf[iBufEnd].data);//TODO:delete
        fflush(stdout);
        ++iBufEnd;
        if (iBufEnd > MAX_BUF_SIZE) {
            printErr(ERR_READ_BUF_OVERFLOW);
        }
        //if receive FIN, terminate printData();
        //use pipe to notify printData data ready
    }
    Pthread_join(tid, NULL);
}

void* printData(void *arg) {
    while (1) {
        //lock iBufEnd
        for (unsigned long int i = iBufBase; i < iBufEnd; ++i) {
            printf("%s", plReadBuf[i].data);
            fflush(stdout);
        }
        iBufBase = iBufEnd;
        struct timespec tm, tmRemain;
        getSleepTime(&tm);
        nanosleep(&tm, &tmRemain);
    }
    return NULL;
}

void getSleepTime(struct timespec* tm) {
    //TODO: turn on rand!!!
    //seed48(config.seed);
    //double r = drand48();
    //printf("%F\n", r);
    double r = 0.45;
    double f = config.mu * log(r); 
    int iMilliSec = -(int)f;
    tm->tv_sec = iMilliSec / 1000;
    tm->tv_nsec = iMilliSec % 1000 * 1000000;
}
