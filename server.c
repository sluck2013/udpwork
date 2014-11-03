#include "unp.h"
#include "server.h"
#include "constants.h"
#include "utility.h"
#include "lib/unprtt.h"
#include "lib/unpifiplus.h"
#include "lib/get_ifi_info_plus.c"
#include <setjmp.h>
#include <math.h>
#include "common.h"
#include <time.h> 

struct server_configuration server_config;
struct socket_configuration socket_config[MAXSOCKET];
int iSockNum;
struct Payload send_buf[MAX_BUF_SIZE];
unsigned char recvd_ack[MAX_BUF_SIZE];
int datagram_num = 0;

static sigjmp_buf jmpbuf;
int rttinit=0;
static struct rtt_info rttinfo;

unsigned long int seqNum = 5;

int main(int argc, char *argv[]) 
{
    /* read file */
    readConfig();

    /* binding each IP address to a distinct UDP socket */
    bindSockets();

    listenSockets();
}


void listenSockets() {
    // use "select " to monitor the sockets it has created
    // for incoming datagrams
    fd_set rset;
    int nready;
    int maxfd;  // maximuim socket file descriptor

    /* to see which file descriptor is the largest */
    maxfd = socket_config[0].sockfd; //assume first one is largest
    for(int socket_cnt = 0; socket_cnt < iSockNum; ++socket_cnt) {
        if(socket_config[socket_cnt].sockfd > maxfd) {
            maxfd = socket_config[socket_cnt].sockfd;
        }
    }

    FD_ZERO(&rset);
    while (1) {
        for(int num = 0; num < iSockNum; ++num) {
            FD_SET(socket_config[num].sockfd, &rset);
        }

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                errQuit("Select error");
            }
        }

        printInfo("HERE");
        //check each interface to see if it can read and 
        //find the one that can read (num)
        for(int num = 0; num < iSockNum; ++num) {
            if(FD_ISSET(socket_config[num].sockfd, &rset)) {
                struct sockaddr_in cliaddr;
                int len_cliaddr = sizeof(cliaddr);
                
                char request_file[MAXLINE];
                /*
                int n = recvfrom(socket_config[num].sockfd, request_file,
                     MAXLINE, 0, (SA*)&cliaddr, &len_cliaddr);
                */

                // receive file name
                struct Payload recvfileBuf;                 
                int n = read(socket_config[num].sockfd, &recvfileBuf, sizeof(recvfileBuf));
                if (n < 0) {
                    errQuit(ERR_READ_DATA_FROM_CLI);
                }

                /*server fork off a child process to handle the client*/
                pid_t pid = fork();
                if (pid < 0) {
                    errQuit(ERR_FORK_FAIL);
                } else if (pid == 0) {
                    handleRequest(num, &cliaddr, request_file);
                    return;
                }
            }
        }
    }
}

void handleRequest(int iListenSockIdx, struct sockaddr_in *pClientAddr, const char *request_file) {
    for (int socket_cnt = 0; socket_cnt < iSockNum; ++socket_cnt) {
        if(socket_cnt != iListenSockIdx) {
            /* close all sockets other than "listening" */
            close(socket_config[socket_cnt].sockfd);
        }
    }

    /* if the client is local,use SO_DONTROUTE socket option */
    int on = 1;
    //char = 1;
    //TODO: uncomment
    if(isLocal(pClientAddr)) {
        printInfo("Client host is local");
        if(setsockopt(socket_config[iListenSockIdx].sockfd, 
                    SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on)) < 0) {
            printInfo("setting socket error ");
            exit(1);
        }
    } else {
        printInfo("client host is not local");
    }

    /*  server child creates "connection" socket */

    int conn_sockfd;
    struct sockaddr_in conn_servaddr;
    //struct sockaddr_in conn_cliaddr;

    conn_sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&conn_servaddr, sizeof(conn_servaddr));
    conn_servaddr.sin_family = AF_INET;
    conn_servaddr.sin_port = htons(0);
    //conn_servaddr.sin_addr = pClientAddr->sin_addr;
    conn_servaddr.sin_addr = socket_config[iListenSockIdx].ip;

    Bind(conn_sockfd, (SA*)&conn_servaddr, sizeof(conn_servaddr));


    // use getsockname 
    socklen_t len_conn_servaddr= sizeof(conn_servaddr);
    int n = getsockname(conn_sockfd, (SA*)&conn_servaddr, &len_conn_servaddr);
    if (n < 0) {
        errQuit(ERR_GETSOCKNAME);
    }

    // get the ephemeral port number 
    char serv_ephe_port[MAX_DATA_LEN];
    sprintf(serv_ephe_port, "%i", ntohs(conn_servaddr.sin_port));
    printSockInfo(&conn_servaddr, "Local");

    //establish connection socket
    Connect(conn_sockfd, (SA*)pClientAddr, sizeof(*pClientAddr));

    //send port to client
    struct Payload newPortPack;
   
    // timeout mechanism initialization for server sending ephemeral port number
    if(rttinit== 0) {
        rtt_init(&rttinfo);
        rttinit = 1;
        rtt_d_flag = 1;
    }
    //signal(SIGALRM, sig_alrm);
    rtt_newpack(&rttinfo);
    packData(&newPortPack, seqNum++, 0, server_config.server_win_size, 0, serv_ephe_port);

LSEND_PORT_AGAIN:
    setPackTime(&newPortPack, rtt_ts(&rttinfo));
    //if (sendto(socket_config[iListenSockIdx].sockfd, &newPortPack,
    //            sizeof(newPortPack), 0, (SA*)pClientAddr, sizeof(*pClientAddr)) < 0) {
    if (write(socket_config[iListenSockIdx].sockfd, &newPortPack, sizeof(newPortPack))) {
        printErr("sending error");
        printErr(strerror(errno));
    } else {
        printItem("send ephemeral port number to client", serv_ephe_port);
    }      
    alarm(rtt_start(&rttinfo));

    if (sigsetjmp(jmpbuf, 1) != 0) {
        if (rtt_timeout(&rttinfo) < 0) {
            printInfo("time out and give up ");
            //if server times out, retransmit via listening socket
            setPackTime(&newPortPack, rtt_ts(&rttinfo) );
            write(conn_sockfd, &newPortPack, sizeof(newPortPack) );
            rttinit = 0;
        }
        goto LSEND_PORT_AGAIN;
    }

    // receive client's ack of getting ephemeral port number and when the server receives ack, closes "listening socket"
    while(1) {
            struct Payload expAck;
            Read(conn_sockfd, &expAck, sizeof(expAck));
            if (isValidAck(&expAck, getSeqNum(&newPortPack))) {
                Close(socket_config[iListenSockIdx].sockfd);
                printInfo("Listening socked closed\n");
                break;
            }
        }
        alarm(0);          //stop timer

        rtt_stop(&rttinfo, rtt_ts(&rttinfo) - getTimestamp(&newPortPack));
    
    printInfo("ephemeral port number transmission is ok ");    

    // transfer file
    printItem("file name", request_file);
    FILE* fileDp;
    fileDp = fopen(request_file, "r");

    if (fileDp == NULL) {
        printInfo("cannot open file!");
        exit(1);
    } 

    while (!feof(fileDp)) {
        int read_num = 0;
        int write_flag = 1;
        char cBuf[MAX_DATA_LEN];

        while (read_num < MAX_DATA_LEN - 1) {
            char c = fgetc(fileDp);
            if (feof(fileDp)) {
                if (read_num == 1) {
                    write_flag = 0;
                } else {
                    write_flag = 1;
                }
                break;
            }
            cBuf[read_num++] = c;
        }
        cBuf[read_num] = '\0';

        if (write_flag) {
            packData(&send_buf[datagram_num++], seqNum++, 0, server_config.server_win_size, 0, cBuf);
        }
    }

    sendData(conn_sockfd, pClientAddr);

}

void readConfig() {
    FILE* config_file = fopen("server.in", "r");
    if (config_file == NULL) {
        errQuit(ERR_OPEN_SERVER_IN);
    }

    int n = fscanf(config_file, "%d", &server_config.server_port);
    if(n < 0) {
        errQuit(ERR_READ_SERVER_PORT);
    }

    int m = fscanf(config_file, "%d", &server_config.server_win_size);
    if(m < 0) {
        errQuit(ERR_READ_SERVER_WIN);
    }

    fclose(config_file);

    printInfoIntItem("server port is: ", server_config.server_port);
    printInfoIntItem("window size is: ", server_config.server_win_size);

    println();
}

void bindSockets() {
    int count = 0;
    const int on = 1;
    //const char on = '1'
    //TODO:Uncomment Solaris

    struct sockaddr_in *sa, *sinptr;  //IP address and network mask for the IP address

    struct ifi_info *ifi = Get_ifi_info_plus(AF_INET, 1);
    struct ifi_info *ifihead = ifi;

    for( ; ifi != NULL; ifi = ifi->ifi_next) {
        /*bind unicast address*/
        socket_config[count].sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
        Setsockopt(socket_config[count].sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

        //sockfd
        sa = (struct sockaddr_in*) ifi->ifi_addr;
        
        sa->sin_family = AF_INET;
        sa->sin_port = htons(server_config.server_port);
        Bind(socket_config[count].sockfd, (SA*)sa, sizeof(*sa));

        //ip address
        socket_config[count].ip = sa->sin_addr;

        // network mask for ip address
        sinptr = (struct sockaddr_in*) ifi->ifi_ntmaddr;
        socket_config[count].mask = sinptr->sin_addr;

        //subnet address
        socket_config[count].subnet = bitwise_and(
                socket_config[count].ip, socket_config[count].mask); 

        printIfiInfo(ifi);
        //printf("Subnet address: %s\n", inet_ntoa(socket_config[count].subnet));
        printItem("Subnet address", inet_ntoa(socket_config[count].subnet) );

        ++count;
    }
    iSockNum = count;
}

struct in_addr bitwise_and(struct in_addr ip, struct in_addr mask) {
    in_addr_t subnet_addr;
    struct in_addr subnet;
    subnet.s_addr=ip.s_addr & mask.s_addr;
    return subnet;
}

int isLocal(struct sockaddr_in *clientAddr) {
    for(int k = 0; k < iSockNum; k++) {
        uint32_t uServerAddr = socket_config[k].ip.s_addr;
        uint32_t uMaskAddr = socket_config[k].mask.s_addr;
        uint32_t uClientAddr = clientAddr->sin_addr.s_addr;

        uint32_t uServerSub = uServerAddr & uMaskAddr;
        uint32_t uClientSub = uClientAddr & uMaskAddr;

        if(uServerSub == uClientSub) {
            return 1;
        }
    }
    return 0;
}

void sendData(int conn_sockfd, struct sockaddr_in *pClientAddr) {
    // timeout mechanism initialization
    if (rttinit == 0) {
        rtt_init(&rttinfo);
        rttinit = 1;
        rtt_d_flag = 1;
    }
    signal(SIGALRM, sig_alrm);
    rtt_newpack(&rttinfo);

    int iBufBase = 0;
    int iBufEnd = 0;
    int iClientBufFull = 0;
    while (iBufBase < datagram_num) {
        if (!iClientBufFull) {
            alarm(rtt_start(&rttinfo));
            if (sigsetjmp(jmpbuf, 1) != 0) {
                if (rtt_timeout(&rttinfo) < 0) {
                    printInfo("time out and give up");
                    rttinit = 0;
                }
                //time out
            }

            for(int i = iBufBase, j = 0; i < datagram_num && j < server_config.server_win_size; ++i, ++j) {
                if (recvd_ack[i]) {
                    continue;
                }
                setPackTime(&send_buf[i], rtt_ts(&rttinfo) );
                write(conn_sockfd, &send_buf[i], sizeof(send_buf[i]));
                ++iBufEnd;
            }
        }

        while(iBufBase < iBufEnd) {
            struct Payload ack;
            read(conn_sockfd, &ack, sizeof(ack));
            if (isValidAck(&ack, 0)) {
                int idx = getAckIdx(&ack, iBufBase);
                if (idx >= iBufEnd || idx < iBufBase) {
                    continue;
                }
                recvd_ack[idx] = 1;
                unsigned long int ackTime = rtt_ts(&rttinfo);
                unsigned long int timeDelta = ackTime - getTimestamp(&send_buf[idx]);
                rtt_stop(&rttinfo, timeDelta);
                if (idx == iBufBase) {
                    while (recvd_ack[iBufBase]) {
                        ++iBufBase;
                    }
                    /* set new alarm on each ack receipt */
                    unsigned long int iElapsed = ackTime - getTimestamp(&send_buf[iBufBase]);
                    unsigned long int iNewDur = rtt_start(&rttinfo);
                    alarm(iNewDur > iElapsed ? iNewDur - iElapsed : 1);
                }
                int iRecvWinSize = getWinSize(&ack);
                printInfoIntItem("receiver's window size", iRecvWinSize);
                if (getWinSize(&ack) == 0) {
                    iClientBufFull = 1;
                }
            }
        }
        
        alarm(0);
    }
    printInfo("file transfer is ok till now");
}

static void sig_alrm(int signo) {
    siglongjmp(jmpbuf, 1);
}

void sig_chld(int signo) {
    pid_t pid;
    int stat;          
    while(  (pid=waitpid(-1, &stat, WNOHANG) ) > 0) {
        printf("child %d terminated\n", pid);
    }
    return;
}

int getAckIdx(const struct Payload* ack, int iBufBase) {
    int iOffset = getAckNum(ack) - getSeqNum(&send_buf[iBufBase]);
    return iBufBase + iOffset;
}
