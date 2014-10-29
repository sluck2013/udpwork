#include"unp.h"
#include"header.h"
#include "constants.h"
#include "unprtt.h"
#include"unpifiplus.h"
#include <setjmp.h>
#include<math.h>


struct server_configuration server_config;
struct in_addr bitwise_and(struct in_addr ip, struct in_addr mask);


int main(int argc, char *argv[]) 
{
       FILE * config_file;
        config_file=fopen("server.in", "r");

         if( config_file==NULL)
         {
         	 printf("server.in file does not exist!\n");
         }

         char char_server_port[MAXLINE];
          int server_port;
          int n=fgets(char_server_port, MAXLINE, config_file);
          if(n<0)
          {
          	printf("reading server port failure!\n");
          }
          else
          	server_port=atoi(char_server_port);

         char char_server_win[MAXLINE];
         int server_win;
         int m=fgets(char_server_win, MAXLINE, config_file);
          if(m<0)
          {
          	printf("reading server window size failure!\n");
          }
          else
          	server_win=atoi(char_server_win);

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
   

   // use select to monitor the sockets it has created for incoming datagrams
   





    



    





         


}



struct in_addr bitwise_and(struct in_addr ip, struct in_addr mask)
{
	struct in_addr subnet;
	subnet.s_addr=ip.s_addr & mask.s_addr;
	return subnet;
}