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

struct server_configuration server_config;
struct socket_configuration socket_config[MAXSOCKET];
int iSockNum;

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

        //check each interface to see if it can read and 
        //find the one that can read (num)
        for(int num = 0; num < iSockNum; ++num) {
            if(FD_ISSET(socket_config[num].sockfd, &rset)) {
                struct sockaddr_in cliaddr;
                int len_cliaddr = sizeof(cliaddr);

                char request_file[MAXLINE];
                int n = Recvfrom(socket_config[num].sockfd, request_file,
                        MAXLINE, 0, (SA*)&cliaddr, &len_cliaddr);
                if (n < 0) {
                    errQuit(ERR_READ_DATA_FROM_CLI);
                }

                /*server fork off a child process to handle the client*/
                pid_t pid = fork();
                if (pid < 0) {
                    errQuit(ERR_FORK_FAIL);
                } else if (pid == 0) {
                    handleRequest(num, &cliaddr,  request_file);

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
    const int on = 1;
    if(isLocal(pClientAddr)) {
        printf("Client host is local\n");
        if(setsockopt(socket_config[iListenSockIdx].sockfd, 
                    SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on)) < 0) {
            printf("setting socket error \n");
            exit(1);
        }
    } else {
        printf("client host is not local\n");
    }

    /*  server child creates "connection" socket */

    int conn_sockfd;
    struct sockaddr_in conn_servaddr;
    //struct sockaddr_in conn_cliaddr;

    conn_sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&conn_servaddr, sizeof(conn_servaddr));
    conn_servaddr.sin_family = AF_INET;
    conn_servaddr.sin_port = htons(0);
    //char IPclient[MAXCHAR], IPserver[MAXCHAR]; 
    //int cli_port_num = atoi(cli_port);
    //sprintf(IPserver, "%s", inet_ntoa(socket_config[num].ip));
    //sprintf(IPclient, "%s", inet_ntoa(cliaddr.sin_addr));
    //char *IPclient = Sock_ntop_host((SA*)&cliaddr.sin_addr, sizeof(cliaddr.sin_addr));
    //char *IPserver = Sock_ntop_host((SA*)&socket_config[num].ip, sizeof(socket_config[num].ip));
    //int k = inet_pton(AF_INET, IPserver, &conn_servaddr.sin_addr);
    conn_servaddr.sin_addr = pClientAddr->sin_addr;
    //if (k <= 0) {
        //printf("Inet_pton error for IPserver\n");
    //    errQuit(ERR_INET_PTON_SERV);
    //}

    Bind(conn_sockfd, (SA*)&conn_servaddr, sizeof(conn_servaddr));


    // use getsockname 
    socklen_t len_conn_servaddr= sizeof(conn_servaddr);
    int n = getsockname(conn_sockfd, (SA*)&conn_servaddr, &len_conn_servaddr);
    if (n < 0) {
        errQuit(ERR_GETSOCKNAME);
    }

    // get the ephemeral port number 

    char serv_ephe_port[MAX_PORT_LEN];
    sprintf(serv_ephe_port, "%i", ntohs(conn_servaddr.sin_port));
    //printf("ephemeral port number is: %s\n", serv_ephe_port);
    
    printSockInfo(&conn_servaddr, "Local");


    //bzero(&conn_cliaddr, sizeof(conn_cliaddr));
    //conn_cliaddr.sin_family = AF_INET;
    //conn_cliaddr.sin_port = cli_port_num;
    //int j = inet_pton(AF_INET, IPclient, &conn_cliaddr.sin_addr);
    //if (j <= 0) {
        //printf("Inet_pton error for IPclient\n");
    //    errQuit(ERR_INET_PTON_CLI);
    //}

    Connect(conn_sockfd, (SA*)pClientAddr, sizeof(*pClientAddr));


    if (sendto(socket_config[iListenSockIdx].sockfd, serv_ephe_port, 
                sizeof(serv_ephe_port), 0, (SA*)pClientAddr, sizeof(*pClientAddr))<0) {
        printf("sending error %s\n", strerror(errno));
    } else {
        printf("send ephemeral port number %s to client \n", serv_ephe_port);
    }             


    close(socket_config[iListenSockIdx].sockfd);

    // transfer file
    //Read(conn_sockfd, request_file, MAXLINE);
    printf("file name: %s\n", request_file);

    FILE* prefiledp;
    prefiledp = fopen(request_file, "r");

    if (prefiledp == NULL) {
        printf("cannot open file!\n");
        exit(1);
    } 

    struct Payload send_buf;
    int data_charNum;
    int send_times;

    while (!feof(prefiledp)) {
        int read_num = 0;
        int send_flag = 1;
        while (read_num < MAX_DATA_LEN - 1) {
            char c = fgetc(prefiledp);
            if (feof(prefiledp)) {
                if (read_num == 1) {
                    send_flag = 0;
                } else {
                    send_flag = 1;
                }
                break;
            }
            send_buf.data[read_num++] = c;
        }
        send_buf.data[read_num] = '\0';

        if (send_flag) {
            //TODO: Set Header
            send_buf.header.seqNum = 1;
            send_buf.header.ack = 0;
            send_buf.header.win = 0;
            Sendto(conn_sockfd, &send_buf, sizeof(send_buf),
                    0, (SA*)pClientAddr, sizeof(*pClientAddr));
        }
    }
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

    printf("server port is : %d\n", server_config.server_port);
    printf("window size is: %d\n", server_config.server_win_size);

    println();
}

void bindSockets() {
    int count = 0;
    const int on = 1;

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

        /*printf("IP address: %s\n", Sock_ntop((SA*)sa, sizeof(* sa)));
        printf("Network mask: %s\n", Sock_ntop((SA*)sinptr, sizeof(*sinptr)));*/
        printIfiInfo(ifi);
        printf("Subnet address: %s\n", inet_ntoa(socket_config[count].subnet));

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

