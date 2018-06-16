#include "csapp.h"

int main(int argc, char** argv) {
    
    struct addrinfo *p, *listp, hints;
    int rc, flags;
    char host[MAXLINE], service[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <dimain name>\n", argv[0]);
    }

    /*get a list of addrinfo records*/
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /*IPv4*/
    hints.ai_socktype = SOCK_STREAM; /*STREAM or DATAGRAM*/
    if ((rc = getaddrinfo(argv[1], NULL, &hints, &listp)) != 0) {
        fprintf(stderr, "getinfo error: %s\n", gai_strerror(rc));
	exit(1);
    }

    /*Walk the list and display each IP address*/
    flags = NI_NUMERICHOST; /*display address string instead of domain name*/
    for (p = listp; p; p = p->ai_next) {
        Getnameinfo(p->ai_addr, sizeof(struct sockaddr), host, MAXLINE, service, MAXLINE, flags);
	printf("%s\n", host);
	printf("%s\n", service);
    }
    
    /*clean up*/
    Freeaddrinfo(listp);
    exit(0);
}
