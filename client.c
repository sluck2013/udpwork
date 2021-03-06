#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include<setjmp.h>
#include "lib/unprtt.h"
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
unsigned long int serverSeqNumStart;
static sigjmp_buf jmpbuf;
int rttinit=0;
static struct rtt_info rttinfo;
char prtMsg[MAXLINE];

pthread_mutex_t iBufBase_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t iBufEnd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t iBufEnd_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t iRecvBufFull_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t iRecvBufFull_cond = PTHREAD_COND_INITIALIZER;
int iRecvBufFull = 0;

int main(int argc, char **argv) {
    //seed48(config.seed);
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
    res[3] = fscanf(fConfig, "%d", &config.recvWinSize);
    res[4] = fscanf(fConfig, "%d", &config.seed);
    res[5] = fscanf(fConfig, "%f", &config.pLoss);
    res[6] = fscanf(fConfig, "%d", &config.mu);
    config.recvWinSize = min(config.recvWinSize, MAX_BUF_SIZE);

    int i;
    for (i = 0; i < 7; ++i) {
        if (res[i] <= 0) {
            errQuit(ERR_READ_CLIENT_IN);
        }
    }
    strcpy(config.IPServer, config.serverAddr);
    fclose(fConfig);

    println();
    printInfo("Client.in read!");
    printf("========== Configure in file ==========\n");
    printf("Server address: %s\n", config.serverAddr);
    printf("Sever's well-know port: %d\n", config.port);
    printf("File to request: %s\n", config.dataFile);
    printf("Receive window size: %d\n", config.recvWinSize);
    printf("Seed: %d\n", config.seed);
    printf("Problability of data loss: %f\n", config.pLoss);
    printf("mu: %d\n", config.mu);
    printf("=======================================\n");
    println();
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
        printInfo("Server is on the same host");
    } else if (isLocal(ifiHead, &ifiMatch)) {
        struct sockaddr *sa = ifiMatch->ifi_addr;
        strcpy(config.IPClient, Sock_ntop_host(sa, sizeof(*sa)));
        printInfo("Server is local");
    } else {
        struct sockaddr *sa = ifiHead->ifi_addr;
        strcpy(config.IPClient, Sock_ntop_host(sa, sizeof(*sa)));
        printInfo("Server is non-local");
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
    struct ifi_info *ifi;
    for (ifi = ifiHead; ifi != NULL; ifi = ifi->ifi_next) {
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

    struct ifi_info *ifi;
    for (ifi = ifiHead; ifi != NULL; ifi = ifi->ifi_next) {
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
    sprintf(prtMsg, "Connected to server %s:%d", config.IPServer, config.port);
    printInfo(prtMsg);

    Getpeername(sockfd, (SA*)&siForeignAddr, &slForeignLen);
    printSockInfo(&siForeignAddr, "Foreign");

    // timeout mechanism initialization for file name transfer to server
    printInfo("Sending request file name to server...");
    struct Payload sendfileBuf;
    
    if (rttinit == 0) {
        rtt_init(&rttinfo);
        rttinit = 1;
        rtt_d_flag = 1;
    }
    signal(SIGALRM, sig_alrm);
    rtt_newpack(&rttinfo);
    packData(&sendfileBuf, seqNum++, 0, config.recvWinSize, 0, config.dataFile);      

LSEND_FILENAME_AGAIN:
    setPackTime(&sendfileBuf, rtt_ts(&rttinfo) );
    Write(sockfd, &sendfileBuf, sizeof(sendfileBuf));
    alarm(rtt_start(&rttinfo));

    if (sigsetjmp(jmpbuf, 1) != 0) {
        if (rtt_timeout(&rttinfo) < 0) {
            rttinit = 0;
        }
        printInfo("Time out, resending file name...");
        goto LSEND_FILENAME_AGAIN;
    }

    struct Payload rawNewPort;

    printInfo("Reading server's port of 'connection' socket");
    while(1) {
        /* get server's "connection" socket port*/
        //struct Payload rawNewPort;
        Read(sockfd, &rawNewPort, sizeof(rawNewPort));
        if (isValidAck(&rawNewPort, getSeqNum(&sendfileBuf))) {
            config.port = atoi(rawNewPort.data);
            serverSeqNumStart = getSeqNum(&rawNewPort) + 1;
            printInfoIntItem("Read port is", config.port);
            break;
        }
    }
    alarm(0);          //stop timer

    rtt_stop(&rttinfo, rtt_ts(&rttinfo) - getTimestamp(&sendfileBuf));

    //reconnect to "connection" socket
    siServerAddr.sin_port = htons(config.port);
    Connect(sockfd, (SA*)&siServerAddr, sizeof(siServerAddr));
    sprintf(prtMsg, "Reconnected to server %s:%d", config.IPServer, config.port);
    printInfo(prtMsg);

    //send back ack of getting ephemeral port number
    struct Payload newPortAck;
    newAck(&newPortAck, seqNum++, getSeqNum(&rawNewPort), min(
               MAX_BUF_SIZE, config.recvWinSize
               ), rawNewPort.header.timestamp
          );
    Write(sockfd, &newPortAck, sizeof(newPortAck));
    printInfo("Sent ack of port info...");

    pthread_t tid;
    struct Arg arg;
    arg.sockfd = sockfd;
    struct Payload ack;

    Pthread_create(&tid, NULL, printData, NULL);
    while (1) {
        Pthread_mutex_lock(&iRecvBufFull_mutex);
        while (iRecvBufFull) { 
            Pthread_cond_wait(&iRecvBufFull_cond, &iRecvBufFull_mutex);
        }
        Pthread_mutex_unlock(&iRecvBufFull_mutex);
        if (getWinSize(&ack) == 0) {
            printInfo("Receiving window reopens. Sending dupliate ACK...");
            setPackWinSize(&ack, getWindowSize());
            Write(sockfd, &ack, sizeof(ack)); 
            printPackInfo(&ack, 2);
        }

        struct Payload msg;
        read(sockfd, &msg, sizeof(msg));
        if(isDropped()) {
            printInfo("Package dropped! (See next line for details)");
            printPackInfo(&msg, 100);
            continue;
        }
        
        Pthread_mutex_lock(&iBufBase_mutex);
        newAck(&ack, seqNum++, getSeqNum(&msg), getWindowSize(), getTimestamp(&msg));
        Pthread_mutex_unlock(&iBufBase_mutex);
        Write(sockfd, &ack, sizeof(ack));
        printInfo("Sent ack");
        printPackInfo(&ack, 2);

        if (iBufEnd > 0) {
            if (getSeqNum(&msg) <= getSeqNum(&plReadBuf[iBufEnd - 1])) {
                continue;
            }
        }

        plReadBuf[getSeqNum(&msg) - serverSeqNumStart] = msg;
        
        Pthread_mutex_lock(&iBufEnd_mutex);
        while(getSeqNum(&plReadBuf[iBufEnd]) > 0) {
            ++iBufEnd;
        }
        if (getWindowSize() == 0) {
            Pthread_mutex_lock(&iRecvBufFull_mutex);
            iRecvBufFull = 1;
            Pthread_mutex_unlock(&iRecvBufFull_mutex);
        }
        Pthread_cond_signal(&iBufEnd_cond);
        Pthread_mutex_unlock(&iBufEnd_mutex);

        if (iBufEnd > MAX_BUF_SIZE) {
            printErr(ERR_READ_BUF_OVERFLOW);
        }

        if (isFin(&msg)) {
            printInfo("Enter TIME_WAIT state");
            alarm(MSL);
            if (sigsetjmp(jmpbuf, 1) != 0) {
                Close(sockfd);
                printInfo("Socket closed!");
                return;
            }
        }
    }
    Pthread_join(tid, NULL);
}

unsigned short int getWindowSize() {
    int endpoint = min(iBufBase + config.recvWinSize, MAX_BUF_SIZE);
    return endpoint > iBufEnd ? endpoint - iBufEnd : 0;
}

void* printData(void *arg) {
    printInfo("Printing data...(printing thread)");
    while (1) {
        Pthread_mutex_lock(&iBufEnd_mutex);
        while (iBufBase >= iBufEnd) {
            Pthread_cond_wait(&iBufEnd_cond, &iBufEnd_mutex);
        }
        //printf("%s\n", plReadBuf[iBufBase].data);
        fflush(stdout);

        Pthread_mutex_lock(&iBufBase_mutex);
        ++iBufBase;
        Pthread_mutex_unlock(&iBufBase_mutex);

        if (getWindowSize() > 0) {
            Pthread_mutex_lock(&iRecvBufFull_mutex);
            iRecvBufFull = 0;
            Pthread_cond_signal(&iRecvBufFull_cond);
            Pthread_mutex_unlock(&iRecvBufFull_mutex);
        }

        struct timespec tm, tmRemain;
        unsigned long int t = getSleepTime(&tm);
        nanosleep(&tm, &tmRemain);
        sprintf(prtMsg, "Sleeping for %d ms...", t);
        printInfo(prtMsg);
        
        Pthread_mutex_unlock(&iBufEnd_mutex);
    }
    return NULL;
}

unsigned long int getSleepTime(struct timespec* tm) {
    float r = drand48();
    float f = config.mu * log(r); 
    int iMilliSec = -(int)f;
    tm->tv_sec = iMilliSec / 1000;
    tm->tv_nsec = iMilliSec % 1000 * 1000000;
    return iMilliSec;
}

static void sig_alrm(int signo) {
    siglongjmp(jmpbuf, 1);
}


int isDropped() {
    float r = drand48();
    if ( r < config.pLoss) {
        return 1;    // drop packet
    }
    else {
        return 0;   // keep packet
    }
}
