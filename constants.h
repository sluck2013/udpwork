#ifndef CONSTANTS_H
#define CONSTANTS_H

/******** ERRORS *******/
#define ERR_OPEN_CLIENT_IN "Cannot open client.in. Make sure it is in the current directory."
#define ERR_READ_CLIENT_IN "Reading client.in"
#define ERR_OPEN_SERVER_IN "Cannot open server.in. Make sure it is in the current directory."
#define ERR_READ_SERVER_PORT "reading server port failure!"
#define ERR_READ_SERVER_WIN "reading server window size failure!"
#define ERR_READ_DATA_FROM_CLI "reading incoming datagram error!" 
#define ERR_FORK_FAIL "fork failed!"
#define ERR_INET_PTON_SERV "Inet_pton error for IPserver"
#define ERR_INET_PTON_CLI "Inet_pton error for IPclient"
#define ERR_GETSOCKNAME "getsockname error!"


/******** OTHER CONSTS ********/
#define MAXCHAR (256)
#define MAXSOCKET (100)

#define MAX_DATA_LEN (500)
#define MAX_PORT_LEN (6)


#endif
