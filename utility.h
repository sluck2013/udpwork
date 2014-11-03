#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include "lib/unpifiplus.h"
#include "common.h"

void printErr(char *errMsg);
void errQuit(char *errMsg);
void printItem(const char* key, const char* value);
void println();
void printInfo(char *info);
void printInfoIntItem(char* item, int info);
void printIfiInfo(struct ifi_info* ifi);
void printSockInfo(struct sockaddr_in* addr, char* addrName);
void packData(struct Payload* datagram, unsigned long int seqNum, 
        unsigned long int ackNum, unsigned short int winSize, 
        unsigned char flag, const char* data);
void newAck(struct Payload* datagram, unsigned long int seqNum,
        unsigned long int ackNum, unsigned short int winSize,
        unsigned long int timestamp);
void setPackTime(struct Payload *datagram, unsigned long int timestamp);
void setPackFlag(struct Payload *datagram, unsigned char flag);
inline int isValidAck(const struct Payload* ack, unsigned long int seqNum) {
    int r = ((ack->header.flag & ACK) == ACK);
    if (seqNum != 0) {
        r = r && (ack->header.ackNum == seqNum);
    }
    return r;
}

inline int isFin(struct Payload *datagram) {
    return ((datagram->header.flag & FIN) == FIN);
}

////////////for debug///////:TODO
void printAddrInfo(SA* addr);
#endif
