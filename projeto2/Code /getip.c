#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

int resolve_hostname_to_ip(const char *hostname, char *ip) {
    struct hostent *he;
    struct in_addr **addr_list;

    if ((he = gethostbyname(hostname)) == NULL) {
        return -1;
    }

    addr_list = (struct in_addr **)he->h_addr_list;

    if (addr_list[0] != NULL) {
        strcpy(ip, inet_ntoa(*addr_list[0]));
        return 0;
    }

    return -1;
}
