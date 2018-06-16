#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);

void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

void build_http_header(char *http_header, char *hostname, char *path, int port, rio_t *client_rio);

void parse_uri(char *uri, char *hostname, char *path, int *port);

int main(int argc, char** argv)
{
    // printf("%s", user_agent_hdr);
    int listenfd, connfd;
    socklen_t clientlen;
    char hostname[MAXLINE], port[MAXLINE];

    struct sockaddr_storage clientaddr;

    if (argc != 2) {
    	fprintf(stderr, "usage: %s <port>\n", argv[0]);
    	exit(1);
    }

    
    listenfd = Open_listenfd(argv[1]);
    while (1) {
    	clientlen = sizeof(clientaddr);
    	connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);

    	/*print accepted message*/
    	Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, 
    		port, MAXLINE, 0);
    	printf("Accepted connection from (%s, %s).\n", hostname, port);

    	/*sequential handle the client transaction*/
    	doit(connfd);

    	Close(connfd);
    }

    return 0;
}

void doit(int connfd) {
	
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	
	//代理服务器要访问的目的网站的主机名, 文件路径, 端口
	char hostname[MAXLINE], path[MAXLINE];
	int port;
	char port_s[MAXLINE];
	int end_serverfd;
	size_t n;

	char endserver_http_header[MAXLINE];

	/*rio is client's rio, server_rio is endserver's rio*/
	rio_t rio, server_rio; 

	//buffered IO 初始化
	Rio_readinitb(&rio, connfd);
	//读到buffer里
	Rio_readlineb(&rio, buf, MAXLINE);
	sscanf(buf, "%s %s %s", method, uri, version);

	/*******debug*******/
	printf("METHOD: %s\n", method);
	printf("URI: %s\n", uri);
	printf("VERSION:%s\n", version);

	//method含有GET(忽略大小写)返回0
	if (strcasecmp(method, "GET")) {
		clienterror(connfd, method, "501", "Not Implemented", 
			"Proxy currently not support this method");
		return;
	}

	//parse the uri -代理服务器收到的uri是完整的ur
	parse_uri(uri, hostname, path, &port);

	/*build the http header which will send to the end server*/
	build_http_header(endserver_http_header, hostname, path, port, &rio);

	sprintf(port_s, "%d", port);
	/*connect to the end server*/

	/*******debug******/
	printf("THE HEADER SEND TO END SERVER\n");
	printf("%s\n", endserver_http_header);

	end_serverfd = Open_clientfd(hostname, port_s);
	
	Rio_readinitb(&server_rio, end_serverfd);
	/*write the http header to end server*/
	Rio_writen(end_serverfd, endserver_http_header, strlen(endserver_http_header));

	while ((n = Rio_readlineb(&server_rio, buf, MAXLINE) != 0)) {
		printf("proxy received %d bytes from end server, then send.\n", (int)n);
		Rio_writen(connfd, buf, n);
	}

	Close(end_serverfd);
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */

/*代理服务器收到的uri是完整的url*/
void parse_uri(char *uri, char *hostname, char *path, int *port) {
	//http服务,默认访问80端口
	*port = 80;
	char *pos = strstr(uri, "//");
	//如uri之前有http:// 则跳过
	pos = (pos != NULL) ? (pos+2) : uri;

	
	char *pos2 = strstr(pos, ":");
	//有:80这种，读取进port www.baidu.com:80/index.html
	if (pos2 != NULL) {
		*pos2 = '\0';
		sscanf(pos, "%s", hostname); //hostname : www.badu.com
		//可以连着读
		sscanf(pos2+1, "%d%s", port, path); //port: 80, path: /index.html
	} else {
		//没有:80, 如www.baidu.com/index.html
		pos2 = strstr(pos, "/");
		if (pos2 != NULL) {
			//有name有path,如www.baidu.com/ (path为 / )
			*pos2 = '\0';
			sscanf(pos, "%s", hostname);
			*pos2 = '/';
			sscanf(pos2, "%s", path);
		} else {
			//没有path,只有hostname 如: www.baidu.com
			sscanf(pos, "%s", hostname);
		}
	}
	return;
}

/*把client发来的首部行和parse的内容填到要发出去的http_header中*/
void build_http_header(char *http_header, char *hostname, char *path, int port, rio_t *client_rio) {
	char buf[MAXLINE] = {'\0'}, request_hdr[MAXLINE] = {'\0'}, other_hdr[MAXLINE] = {'\0'}, host_hdr[MAXLINE] = {'\0'};
	/*request line*/
	/*向end server仅仅发HTTP/1.0*/
	sprintf(request_hdr, "GET %s HTTP/1.0\r\n", path);

	/*request headers*/
	while (Rio_readlineb(client_rio, buf, MAXLINE) > 0) {
		if (strcmp(buf, "\r\n") == 0) {
			break;
		}

		if (!strcasecmp(buf, "Host")) {
			sprintf(host_hdr, "Host: %s\r\n", hostname);
		}

		if (strcasecmp(buf, "Connection") &&
			strcasecmp(buf, "Proxy-Connection") &&
			strcasecmp(buf, "User-Agent") &&
			strcasecmp(buf, "Host")) {
			strcat(other_hdr, buf);
		}
	}

	//如果client发来的request header根本没有Host,也要生成一个
	if (strlen(host_hdr) == 0) {
		sprintf(host_hdr, "Host: %s\r\n", hostname);
	}

	sprintf(http_header, "%s%s%s%s%s%s%s",
		request_hdr,
		host_hdr,
		"Connection: close\r\n",
		"Proxy-Connection: close\r\n",
		user_agent_hdr,
		other_hdr,
		"\r\n");
}