//#include <stdio.h>
#include <stdlib.h>
#include "utility.h"
#include "lib/unpifiplus.h"
#include "unp.h"
#include "time.h"

void printErr(char *errMsg) {
    fprintf(stderr, "ERROR: %s\n", errMsg);
}

void errQuit(char *errMsg) {
    printErr(errMsg);
    exit(0);
}

void printItem(const char* key, const char* value) {
//time_t clock;
//time(&clock);
//struct tm* t = localtime(&clock);
    printf("%s: %s\n", key, value);
    fflush(stdout);
}

void println() {
    printf("\n");
    fflush(stdout);
}

void printInfo(char* info) {
    printItem("INFO", info);
}

void printInfoIntItem(char* item, int info) {
    printf("INFO: %s: %d\n", item, info);
    fflush(stdout);
}

void printIfiInfo(struct ifi_info* ifi) {
    struct sockaddr *sa = ifi->ifi_addr;
    //printItem("Interface name", ifi->ifi_name);
    printItem("IP address", Sock_ntop_host(sa, sizeof(*sa)));
    sa = ifi->ifi_ntmaddr;
    printItem("Network mask", Sock_ntop_host(sa, sizeof(*sa)));
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
    printf("INFO: %s address assigned to socket: %s:%u\n", addrName, pcIP, uiPort);
}

/*
 * Pack data in to payload.
 * @param struct Payload* datagram, pointer to the payload to be
 *        packed into
 * @param char* data pointer to data read from file
 */
void packData(struct Payload* datagram, unsigned long int seqNum,
      unsigned long int ackNum, unsigned short int winSize,
      unsigned char flag,  const char* data) {
    datagram->header.seqNum = seqNum;
    datagram->header.timestamp = 0;
    datagram->header.ackNum = ackNum;
    datagram->header.winSize = winSize;
    datagram->header.flag = flag;
    strcpy(datagram->data, data);
}

void setPackTime(struct Payload *datagram, unsigned long int timestamp) {
    datagram->header.timestamp = timestamp;
}

void setPackFlag(struct Payload *datagram, unsigned char flag) {
    datagram->header.flag = flag;
}

void setPackWinSize(struct Payload *datagram, short int winSize) {
    datagram->header.winSize = winSize;
}

void newAck(struct Payload* datagram, unsigned long int seqNum,
        unsigned long int ackNum, unsigned short int winSize,
        unsigned long int timestamp) {
    unsigned char flag = 1 << 7;
    packData(datagram, seqNum, ackNum, winSize, flag, " ");
    setPackTime(datagram, timestamp);
}

unsigned long int getSeqNum(const struct Payload* datagram) {
    return datagram->header.seqNum;
}

unsigned long int getTimestamp(const struct Payload *datagram) {
    return datagram->header.timestamp;
}

unsigned long int getAckNum(const struct Payload* datagram) {
    return datagram->header.ackNum;
}

unsigned long int getWinSize(const struct Payload* datagram) {
    return datagram->header.winSize;
}

void printPackInfo(const struct Payload* datagram, int mode) {
    char key[MAXLINE];
    switch (mode) {
        case 0:
            strcpy(key, "SENT DATA");
            break;
        case 1:
            strcpy(key, "RECV DATA");
            break;
        case 2:
            strcpy(key, "SENT ACK ");
            break;
        case 3:
            strcpy(key, "RECV ACK ");
            break;
        default:
            strcpy(key, "DATA");
    }
    printf("%s: SEQ#:%lu, ACK#:%lu, TIMESTAMP:%lu, WINSIZE:%d, FIN:%d, ACK:%d\n", key,  getSeqNum(datagram), getAckNum(datagram), getTimestamp(datagram), getWinSize(datagram), isFin(datagram), isValidAck(datagram, 0));
}

int isValidAck(const struct Payload* ack, unsigned long int seqNum) {
    int r = ((ack->header.flag & ACK) == ACK);
    if (seqNum != 0) {
        r = r && (ack->header.ackNum == seqNum);
    }
    return r;
}

int isFin(struct Payload *datagram) {
    return ((datagram->header.flag & FIN) == FIN);
}

///////////////////for debug use///////////////:TODO
void printAddrInfo(SA *addr) {
    struct sockaddr_in *si = (struct sockaddr_in*)addr;
    int port = ntohs(si->sin_port);
    char* ip = Sock_ntop_host(addr, sizeof(*addr));
    printf("DEBUG: %s:%d\n", ip, port);
    fflush(stdout);
}
