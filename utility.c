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
}

void println() {
    printf("\n");
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
