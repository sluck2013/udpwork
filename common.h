#ifndef COMMON_H
#define COMMON_H

struct protHeader {
	unsigned int seqNum;
	int ack;
	int win;
};

struct payload {
	struct protHeader;
	byte data[MAX_DATA_LEN];
#endif
