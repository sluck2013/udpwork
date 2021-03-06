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
void printPackInfo(const struct Payload* datagram, int mode);
void packData(struct Payload* datagram, unsigned long int seqNum, 
        unsigned long int ackNum, unsigned short int winSize, 
        unsigned char flag, const char* data);
void newAck(struct Payload* datagram, unsigned long int seqNum,
        unsigned long int ackNum, unsigned short int winSize,
        unsigned long int timestamp);
void setPackTime(struct Payload *datagram, unsigned long int timestamp);
void setPackFlag(struct Payload *datagram, unsigned char flag);
int isValidAck(const struct Payload* ack, unsigned long int seqNum);
int isFin(struct Payload *datagram);

////////////for debug///////:TODO
void printAddrInfo(SA* addr);
#endif
