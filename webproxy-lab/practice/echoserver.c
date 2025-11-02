#include "csapp.h"
#include <stdio.h>


void echo(int connfd);

int main(int argc, char **argv){
    /* 초기화 */
    int listen_fd, conn_fd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    /* 입력값 검증*/
    if(argc != 2){
        fprintf(stderr, "사용법을 확인하세요 : %s <port>\n", argv[0]);
    }

    /* 루프 수행 */
    listen_fd = Open_listenfd(argv[1]);
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        conn_fd = Accept(listen_fd, &clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s) \n", client_hostname, client_port);
        echo(conn_fd);
        close(conn_fd);
    }
    exit(0);
}