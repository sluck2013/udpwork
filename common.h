#ifndef COMMON_H
#define COMMON_H

struct ProtHeader {
	uint32_t seqNum;
	int ack;
	int win;
};

struct Payload {
	struct ProtHeader header;
	//byte data[MAX_DATA_LEN];
   	 char data[MAX_DATA_LEN];
};

struct Ack {
	uint32_t seqNum;
	uint32_t timestamp;
};

#endif
