#include "csapp.h"

int main(int argc, char const *argv[])
{
	int clientfd;
	char *host, *port; 
	char buf[MAXLINE];
	rio_t rio;

	if (arc != 3) {
		fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
	}

	host = argv[1];
	port = argv[2];

	// clientfd = Socket(AF_INET, SOCK_STREAM, 0);
	// connect(clientfd, sockaddr, sizeof(sockaddr_in));
	clientfd = Open_clientfd(host, port);
	Rio_readinitb(&rio, clientfd);

	while (Fgets(buf, MAXLINE, stdin) != NULL) {
		Rio_writen(clientfd, buf, strlen(buf));
		Rio_readlineb(&rio, buf, MAXLINE);
		Fputs(buf, stdout);
	}

	Close(clientfd);
	exit(0);
}