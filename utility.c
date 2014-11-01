//#include <stdio.h>
#include <stdlib.h>
#include "utility.h"
#include "lib/unpifiplus.h"
#include "unp.h"

void printErr(char *errMsg) {
    fprintf(stderr, "ERROR: %s\n", errMsg);
}

void errQuit(char *errMsg) {
    printErr(errMsg);
    exit(0);
}

void printItem(const char* key, const char* value) {
    printf("%s: %s\n", key, value);
    fflush(stdout);
}

void println() {
    printf("\n");
    fflush(stdout);
}

void printInfo(char* info) {
    printItem("INFO", info);
}

void printIfiInfo(struct ifi_info* ifi) {
    struct sockaddr *sa = ifi->ifi_addr;
    //printItem("Interface name", ifi->ifi_name);
    printItem("IP address", Sock_ntop_host(sa, sizeof(*sa)));
    sa = ifi->ifi_ntmaddr;
    printItem("Network mask", Sock_ntop_host(sa, sizeof(*sa)));
}

/*
 * Print IP address and port number from sockaddr_in struct.
 * Used to print info returned by getsockname and getpeername.
 * @param struct sockaddr_in* addr pointer to sockaddr_in variable
 *        containing IP server and port number
 * @param char* addrName string which should be either "Local"
 *        or "Foreign"
 */
void printSockInfo(struct sockaddr_in* addr, char* addrName) {
    unsigned int uiPort = ntohs(addr->sin_port);
    char* pcIP = Sock_ntop_host((SA*)addr, sizeof(*addr));
    printf("%s address assigned to socket: %s:%u\n", addrName, pcIP, uiPort);
}

/*
 * Pack data in to payload.
 * @param struct Payload* datagram, pointer to the payload to be
 *        packed into
 * @param char* data pointer to data read from file
 */
void packData(struct Payload* datagram, char* data) {
    //TODO: pack
    datagram->header.seqNum = 0;
    datagram->header.timestamp = 0;
    datagram->header.ackNum = 0;
    datagram->header.winSize = 0;
    strcpy(datagram->data, data);
}
