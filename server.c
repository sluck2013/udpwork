#include "unp.h"
#include "server.h"
#include "constants.h"
#include "utility.h"
#include "lib/unprtt.h"
#include "lib/unpifiplus.h"
#include "lib/get_ifi_info_plus.c"
#include <setjmp.h>
#include <math.h>


struct server_configuration server_config;
struct in_addr bitwise_and(struct in_addr ip, struct in_addr mask);
struct socket_configuration socket_config[MAXSOCKET];
void readConfig();
int bindSockets();

int main(int argc, char *argv[]) 
{
    int socket_cnt;

    /* read file */
    readConfig();

    /* binding each IP address to a distinct UDP socket */
    int iSockNum = bindSockets();

    // use "select " to monitor the sockets it has created
    // for incoming datagrams
    fd_set rset;
    int nready;
    int maxfd;  // maximuim socket file descriptor

    /* to see which file descriptor is the largest */
    maxfd = socket_config[0].sockfd; //assume first one is largest
    for(socket_cnt = 0; socket_cnt < iSockNum; ++socket_cnt) {
        if(socket_config[socket_cnt].sockfd > maxfd) {
            maxfd = socket_config[socket_cnt].sockfd;
        }
    }


    FD_ZERO(&rset);
    while(1) {
        int num;
        for(num = 0; num < iSockNum; ++num) {
            FD_SET(socket_config[num].sockfd, &rset);
        }


        nready = Select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            if(errno == EINTR) {
                continue;
            } else {
                exit(1);
            }
        }

        //check each interface to see if it can read and 
        //find the one that can read (num)
        for(num = 0; num < iSockNum; ++num) {
            if(FD_ISSET(socket_config[num].sockfd, &rset)) {
                struct sockaddr_in cliaddr;
                int len_cliaddr = sizeof(cliaddr);

                char cli_port[MAXLINE];
                int n = Recvfrom(socket_config[num].sockfd, cli_port, 
                        MAXLINE, 0, (struct sockaddr*)&cliaddr, &len_cliaddr);
                if (n < 0) {
                    errQuit(ERR_READ_DATA_FROM_CLI);
                }

                char IPclient[MAXLINE], IPserver[MAXLINE]; 
                int cli_port_num;

                cli_port_num = atoi(cli_port);
                sprintf(IPserver, "%s", inet_ntoa(socket_config[num].ip));
                sprintf(IPclient, "%s", inet_ntoa(cliaddr.sin_addr));

                /*server fork off a child process to handle the client*/
                pid_t pid;
                pid = fork();

                if (pid < 0) {
                    errQuit(ERR_FORK_FAIL);
                }

                if (pid == 0) {
                    for (socket_cnt = 0; socket_cnt<iSockNum; ++socket_cnt) {
                        if(socket_cnt != num) {
                            /*close all the sockets except the one on which the client request arrived (num)
                              leave the "listening" socket open*/
                            close(socket_config[socket_cnt].sockfd);
                        }
                    }

                    //check if client host is local
                    /* Determine if the client is on the same local network */
                    int isLocal;
                    for(int k = 0; k < iSockNum; k++)
                    {

                        uint32_t uServerAddr = socket_config[k].ip.s_addr;
                        uint32_t uMaskAddr = socket_config[k].mask.s_addr;
                        uint32_t uClientAddr = cliaddr.sin_addr.s_addr;

                        uint32_t uServerSub = uServerAddr & uMaskAddr;
                        uint32_t uClientSub = uClientAddr & uMaskAddr;


                        //if the XOR is = 0, then perfect match, and therefore local
                        if(uServerSub == uClientSub)
                        {
                            isLocal = 1;
                            break;
                        }
                    }



                    /* if the client is on the local net, then use the SO_DONTROUTE socket option */
                    int on = 1;
                    if(isLocal == 1)
                    {
                        printf("*client host is local\n");
                        if(setsockopt(socket_config[num].sockfd, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on)) < 0)
                        {
                            printf("setting socket error \n");
                            exit(1);
                        }
                    }
                    else
                    {
                        printf("*client host is not local\n");
                    }







                    /*  server child creates a UDP socket(connection socket) 
                        to handle file transfer to client  */

                    int conn_sockfd;
                    struct sockaddr_in conn_servaddr;
                    struct sockaddr_in conn_cliaddr;

                    conn_sockfd=Socket(AF_INET, SOCK_DGRAM, 0);

                    bzero(&conn_servaddr, sizeof(conn_servaddr));
                    conn_servaddr.sin_family=AF_INET;
                    conn_servaddr.sin_port=htons(0);
                    int k=inet_pton(AF_INET, IPserver, &conn_servaddr.sin_addr);
                    if(k<=0)
                    {
                        //printf("Inet_pton error for IPserver\n");
                        errQuit(ERR_INET_PTON_SERV);
                    }

                    Bind(conn_sockfd, (SA *) &conn_servaddr, sizeof(conn_servaddr));


                    // use getsockname 
                    socklen_t len_conn_servaddr= sizeof(conn_servaddr);
                    int n= getsockname(conn_sockfd, (SA* )& conn_servaddr, &len_conn_servaddr);
                    if(n<0)
                    {
                        errQuit(ERR_GETSOCKNAME);
                    }

                    // get the ephemeral port number 

                    char  serv_ephe_port[MAXLINE];
                    sprintf(serv_ephe_port, "%i", ntohs(conn_servaddr.sin_port));
                    printf("ephemeral port number is: %s\n",  serv_ephe_port);

                    Write(conn_sockfd, serv_ephe_port, MAXLINE);
                    close(socket_config[num].sockfd);


                    bzero(&conn_cliaddr, sizeof(conn_cliaddr));
                    conn_cliaddr.sin_family=AF_INET;
                    conn_cliaddr.sin_port=cli_port_num;
                    int j=inet_pton(AF_INET, IPclient, &conn_cliaddr.sin_addr);
                    if(j<=0)
                    {
                        //printf("Inet_pton error for IPclient\n");
                        errQuit(ERR_INET_PTON_CLI);
                    }

                    Connect(conn_sockfd, (SA *)&conn_cliaddr, sizeof(conn_cliaddr));


                }



            }
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

int bindSockets() {
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
    return count;
}



struct in_addr bitwise_and(struct in_addr ip, struct in_addr mask)
{
    in_addr_t subnet_addr;
    struct in_addr subnet;
    subnet.s_addr=ip.s_addr & mask.s_addr;
    return subnet;
}
