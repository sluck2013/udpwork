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
#define ERR_READ_BUF_OVERFLOW "Received data exceeds reading buffer capacity."

/******** OTHER CONSTS ********/
#define MAXCHAR (256)
#define MAXSOCKET (100)

#define MAX_PORT_LEN (6)
//#define MAX_DATA_LEN (10)
//#define PAYLOAD_SIZE (25)
#define MAX_DATA_LEN (407)
#define PAYLOAD_SIZE (512)
#define MAX_BUF_SIZE (500)

#define ACK (128)
#define FIN (64)
#define MSL (30)

#endif
