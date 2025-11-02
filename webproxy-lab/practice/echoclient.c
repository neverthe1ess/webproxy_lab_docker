#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "csapp.h"

int main(int argc, char **argv){
    int client_fd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if(argc != 3){
        fprintf(stderr, "사용법을 확인하세요 : %s <host><port>\n", argv[0]);
    }
    host = argv[1];
    port = argv[2];

    client_fd = Open_clientfd(host, port);
    Rio_readinitb(&rio, client_fd);

    while(Fgets(buf, MAXLINE, stdin) != NULL){
        Rio_writen(client_fd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(client_fd);
    exit(0);
}