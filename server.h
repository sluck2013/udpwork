#include <math.h>
#include <stdlib.h>
#include "unp.h"
#include "lib/unpifiplus.h"
#include "lib/unprtt.h"

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
    int server_win_size;
};

void readConfig();
void bindSockets();
void listenSockets();
int isLocal(struct sockaddr_in *clientAddr);
void handleRequest(int iListenSock, struct sockaddr_in *pcClientAddr);
struct in_addr bitwise_and(struct in_addr ip, struct in_addr mask);
