#include <string.h>
#include "util.h"

/**
 * 함수의 책임 : 호스트 이름과 포트를 받아 클라이언트용 연결 소켓을 제공한다. 
 */

int Open_clientfd(char *hostname, char *port){
    int client_fd;
    struct addrinfo hints, *listp, *p;

    /* hint 구조체 초기화 */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    Getaddrinfo(hostname, port, &hints, &listp);

    for (p = listp; p; p = p -> ai_next)
    {
        if((client_fd = socket(p ->ai_family, p ->ai_socktype, p -> ai_protocol)) < 0){
            continue;
        }

        if(connect(client_fd, p -> ai_addr, p -> ai_addrlen) != -1){
            break;
        }
        close(client_fd);
    }

    freeaddrinfo(listp);
    if(!p){
        /* 끝까지 찾아도 안나오면 */
        return -1;
    } else {
        return client_fd;
    }
}

/**
 * 함수의 책임 : 포트 번호를 받아서 연결 요청을 받을 듣기 소켓을 만들어 제공한다.
 */

int Open_listenfd(char *port){
    int listen_fd, optval = 1;
    struct addrinfo hints, *listp, *p;

    /* 힌트 구조체 초기화 */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;

    /* 초기 소켓 -> 소켓을 소켓 주소 구조체와 바인딩 -> 듣기 소켓 */
    Getaddrinfo(NULL, port, &hints, &listp);

    for(p = listp; p; p = p -> ai_next){
        if((listen_fd = socket(p ->ai_family, p ->ai_socktype, p ->ai_protocol)) < 0){
            continue;
        }
        Setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        if(bind(listen_fd, p -> ai_addr, p -> ai_addrlen) == 0)
            break;
        Close(listen_fd);
    }

    /* Clean up */
    Freeaddrinfo(listp);
    if(!p){
        return -1;
    }

    if(listen(listen_fd, LISTENQ) < 0){
        Close(listen_fd);
        return -1;
    }
    return listen_fd;
}