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


int main(int argc, char *argv[]) 
{
       int socket_cnt;
       FILE * config_file;
        config_file=fopen("server.in", "r");

         if( config_file==NULL)
         {
         	 errQuit(ERR_OPEN_SERVER_IN);
         }

         //char char_server_port[MAXLINE];
          int server_port;
         // int n=fgets(char_server_port, MAXLINE, config_file);
             int n=fscanf(config_file, "%d",  &server_port);
          if(n<0)
          {
          	errQuit(ERR_READ_SERVER_PORT);
          }
          //else
          	//server_port=atoi(char_server_port);

         //char char_server_win[MAXLINE];
         int server_win;
         //int m=fgets(char_server_win, MAXLINE, config_file);
          int m=fscanf(config_file, "%d",  &server_win);
          if(m<0)
          {
          	errQuit(ERR_READ_SERVER_WIN);
          }
          //else
          //	server_win=atoi(char_server_win);

          fclose(config_file);

    server_config.server_port=server_port;
    printf("server port is : %d\n", server_port);

    server_config.server_win_size=server_win;
    printf("window size is: %d\n", server_win);

    printf("\n\n");


    /* replacement of IP_RECVDESTADDR    P609
         binding each IP address to a distinct UDP socket     */
    int sockfd;
    int  count=0;
    const int on=1;

    struct sockaddr_in *sa, *sinptr;     //  IP address and network mask for the IP address
    struct socket_configuration socket_config[MAXLINE];

    struct ifi_info *ifi, *ifihead;

    for(ifihead=ifi=Get_ifi_info_plus(AF_INET,1) ; ifi!=NULL; ifi=ifi->ifi_next)
    {
           /*bind unicast address*/
           sockfd= Socket(AF_INET, SOCK_DGRAM, 0);
           Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
           
          // sockfd
           socket_config[count].sockfd=sockfd;
           sa = (struct sockaddr_in *) ifi->ifi_addr;
           sa->sin_family = AF_INET;
           sa->sin_port = htons(server_port);
           Bind(socket_config[count].sockfd, (SA *) sa, sizeof(*sa) );

           socket_config[count].ip=sa->sin_addr;    // ip address 
           // network mask for ip address
           sinptr=(struct sockaddr_in *) ifi->ifi_ntmaddr;
           socket_config[count].mask=sinptr->sin_addr;
           //subnet address
           socket_config[count].subnet=bitwise_and(socket_config[count].ip, socket_config[count].mask); 

         printf("  IP address is : %s\n", Sock_ntop((SA  *) sa, sizeof(* sa)));
         printf("Network mask is : %s\n", Sock_ntop((SA*) sinptr, sizeof(*sinptr)));
         printf(" Subnet address is: %s\n", inet_ntoa(socket_config[count].subnet));

         count+=1;
     }
   

   // use "select " to monitor the sockets it has created for incoming datagrams
    fd_set rset;
    int nready;
    int maxfd;  // maximuim socket file descriptor
    
     /* to see which file descriptor is the largest */
      maxfd=socket_config[0].sockfd;      // assume the first one is the largest
      for( socket_cnt=0; socket_cnt<count; socket_cnt++)
      {
      	if(socket_config[socket_cnt].sockfd>maxfd)
      	      {
      		      maxfd=socket_config[socket_cnt].sockfd;
      		}
      }


     FD_ZERO(&rset);
      while(1)
      {
           int  num;
           for(num=0; num<count; num++)
           {
                FD_SET(socket_config[num].sockfd, &rset);
           }


             nready=Select(maxfd+1,&rset,NULL,NULL,NULL);
             if(nready<0)
             {
                   if(errno==EINTR)
                   {
                         continue;
                   }
                   else
                   {
                      exit(1);
                   }
             }

             //check each interface to see if it can read and find the one that can read (num)
            for(num=0; num<count; num++)
            {
              if(FD_ISSET(socket_config[num].sockfd, &rset))
              {
                   struct sockaddr_in   cliaddr;
                   int len_cliaddr= sizeof( cliaddr );

                   char cli_port[MAXLINE];
                   int n=Recvfrom(socket_config[num].sockfd, cli_port, MAXLINE, 0, (struct  sockaddr *) &cliaddr, &len_cliaddr );

                   if(n<0)
                   {
                    errQuit(ERR_READ_DATA_FROM_CLI);
                    //printf("reading incoming datagram error!\n");
                   }

                   char IPclient[MAXLINE], IPserver[MAXLINE]; 
                   int cli_port_num;

                   cli_port_num=atoi(cli_port);
                   sprintf(IPserver, "%s", inet_ntoa(socket_config[num].ip));
                   sprintf(IPclient, "%s", inet_ntoa(cliaddr.sin_addr));



                /*server fork off a child process to handle the client*/
                
                pid_t pid;
                pid=fork();
                
                if(pid<0)
                {
                  errQuit(ERR_FORK_FAIL);
                  //printf("fork failed!\n");
                }

                if(pid==0)
                {
                      for(socket_cnt=0; socket_cnt<count; socket_cnt++)
                      {
                        if(socket_cnt!=num)
                             {
                                /*close all the sockets except the one on which the client request arrived (num)*/
                                 close(socket_config[socket_cnt].sockfd);  
                              }
                      }
                    


                }



              }
            }
   

      }




    



    





         


}







struct in_addr bitwise_and(struct in_addr ip, struct in_addr mask)
{
      in_addr_t subnet_addr;
	struct in_addr subnet;
	subnet.s_addr=ip.s_addr & mask.s_addr;
	return subnet;
}
