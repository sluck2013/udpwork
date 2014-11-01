#ifndef COMMON_H
#define COMMON_H

#include "constants.h"

struct ProtHeader {
	unsigned long int seqNum;
	unsigned long int timestamp;
    unsigned long int ackNum;
    unsigned short int winSize;
    unsigned char flag;
};

struct Payload {
	struct ProtHeader header;
   	char data[MAX_DATA_LEN];
};

#endif
