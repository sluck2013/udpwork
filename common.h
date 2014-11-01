#ifndef COMMON_H
#define COMMON_H

struct ProtHeader {
	unsigned int seqNum;
	int ack;
	int win;
};


struct Payload {
	struct ProtHeader header;
	//byte data[MAX_DATA_LEN];
    char data[MAX_DATA_LEN];
};


#endif
