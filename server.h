#ifndef SERVER_H
#define SERVER_H

#include <math.h>
#include <stdlib.h>
#include "unp.h"
#include "lib/unpifiplus.h"
#include "lib/unprtt.h"
#include "common.h"

struct socket_configuration
{
   int sockfd;
   struct in_addr ip; 
   struct in_addr mask;
   struct in_addr subnet;
};


struct server_configuration
{
    int server_port; 
    unsigned int server_win_size;
};

void readConfig();
void bindSockets();
void listenSockets();
int isLocal(struct sockaddr_in *clientAddr);
void handleRequest(int iListenSock, struct sockaddr_in *pcClientAddr, const char* request_file, unsigned long int fnameSeqNum);
struct in_addr bitwise_and(struct in_addr ip, struct in_addr mask);
void sendData(int conn_sockfd, struct sockaddr_in *pClientAddr);

static void sig_alrm(int signo);
void sig_chld(int signo);

#endif
