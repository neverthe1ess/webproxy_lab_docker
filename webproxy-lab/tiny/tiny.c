/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, int is_head);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  /* 듣기 소켓을 오픈한다. */
  listenfd = Open_listenfd(argv[1]);
  /* 무한 루프 */
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

/** 
 * doit 함수는 한 개의 HTTP 트랜잭션을 처리한다. 
 * flow : 요청 라인을 읽고 -> 파싱 -> 응답 전송
 * 제약 조건 : GET, HEAD 메소드만 지원, 클라이언트가 다른 메소드(POST 같은)를 요청하면, 에러 메시지를 보낸다.
*/

/**
 * brief 연결된 소켓에서 한 개의 HTTP 트랜잭션을 처리한다.
 * param fd이며, 클라이언트와 연결된 소켓이다.
 * pre  fd는 유효해야 하며, 읽기/쓰기 가능해야 한다. 요청은 CRLF로 끝나야 한다.
 * post 항상 
 * error 400/403/404/501를 clienterror()를 통해 전송한다.
 * sidee
 */

void doit(int fd){
 
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  struct stat sbuf;
  rio_t rio;

  Rio_readinitb(&rio, fd);
  /* 응용 버퍼에서 목적지로 전송 */
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("요청 헤더를 읽어봅시다. \n");
  printf("%s\n", buf);
  /* 응답 라인에서 토큰나이징을 위해 구분자 찾기 */
  char *dem_p1 = strchr(buf, ' ');
  char *dem_p2 = dem_p1 ? strchr(dem_p1 + 1, ' ') : NULL;
  
  if (!dem_p1 || !dem_p2) {
    clienterror(fd, buf, "400", "Bad Request", "Malformed request line");
    return;
  }

  size_t method_len = (size_t)(dem_p1 - buf);
  size_t uri_len = (size_t)(dem_p2 - (dem_p1 + 1));
  char *vstart = dem_p2 + 1;
  char *vend = strchr(vstart, '\r');
  size_t ver_len = vend ? (size_t)(vend - vstart) : strlen(vstart);

  if(method_len == 0 || uri_len == 0 || ver_len == 0){
    clienterror(fd, buf, "400", "Bad Request", "Request could not be understood by the server.");
    return;
  }

  if(method_len >= MAXLINE || uri_len >= MAXLINE || ver_len >= MAXLINE){
    clienterror(fd, buf, "400", "Bad Request", "Request could not be understood by the server.");
    return;
  }

  memcpy(method, buf, method_len);
  method[method_len] = '\0';
  memcpy(uri, dem_p1 + 1, uri_len);
  uri[uri_len] = '\0';
  memcpy(version, dem_p2 + 1, ver_len);
  version[ver_len] = '\0';

  if(strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0){
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  /* 요청 헤더를 무시함 */
  read_requesthdrs(&rio);

  /* uri에서 파일 이름과 인자 분리, CGI 스트링 분석하기 (정적 / 동적인지 확인하기) */
  int is_static = parse_uri(uri, filename, cgiargs);
  /* 실제 디스크에 파일이 있는지 확인*/
  if(stat(filename, &sbuf) < 0){
    clienterror(fd, filename, "404", "Not Found", "Tiny Couldn't read the file");
    perror("stat : ");
    return;
  }

  int is_head = (strcmp(method, "HEAD") == 0);

  /* 정적 / 동적 구별하고 일반 파일이면서 읽기 또는 실행 권한 있는지 확인 */
  if(is_static){
    if(!S_ISREG(sbuf.st_mode) || !(sbuf.st_mode & S_IRUSR)){
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    /* 정적 콘텐츠 제공*/
    serve_static(fd, filename, sbuf.st_size, is_head);
  } else {
    if(!S_ISREG(sbuf.st_mode) || !(sbuf.st_mode & S_IXUSR)){
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, is_head);
  }
  return;
}
/**
 * param : rp - 이미 rio_readinitb를 통해 소켓과 버퍼가 연결됨
 * pre : 요청라인은 이미 읽혀 있고, 현재 위치는 첫번째 헤더라인
 * post : 빈 줄 직후까지 전진한다. 즉, 마지막 \r\n 읽으면 끝
 */

void read_requesthdrs(rio_t *rp){
  char buf[MAXLINE];

  do{
    Rio_readlineb(rp, buf, MAXLINE);
    //printf("%s", buf);
  } while(strcmp(buf, "\r\n"));
  return;
}

/**
 * 에러 발생 시 클라이언트에게 HTTP 응답을 응답 라인에 적절한 상태 코드와 상태 메시지와 함께 보낸다.
 * HTML 응답은 MIME 타입, 길이가 필수 길이를 알기 쉬운 HTML로 전송
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
  char buf[MAXLINE], body[MAXLINE];

  /* HTTP Response Body*/
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""#ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);

  /* HTTP Response Line + Header + Empty Line */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));

}

/**
 * uri에서 파일 이름과 인자를 분리한다.
 * /cgi-bin로 구성되면 동적 컨텐츠, 아니면 정적 콘텐츠로 판단한다.
 * 정적 컨텐츠를 위한 홈 디렉토리가 자신의 현재 디렉터리이고, 실행파일의 홈 디렉토리는 /cgi-bin로 가정한다.
 */
int parse_uri(char *uri, char *filename, char *cgiargs){
  /* 정적 컨텐츠면 cgi 관련 인자는 empty, 파일 이름(경로)는 /test.html -> ./test.html로 변경, 만약 /이면 기본 파일 이름을 추가*/
  if(!strstr(uri, "/cgi-bin/")){
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri) - 1] == '/'){
      strcat(filename, "home.html");
    }
    return 1;
  } else {
    /* 동적 컨텐츠면 cgi 인자와 파일 이름 분리(상대 리눅스 경로)*/
    char *ptr = strchr(uri, '?');
    if(ptr == NULL){
      strcpy(cgiargs, "");
    } else {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}


/**
 * 역할 : 로컬 파일 filename을 HTTP/1.0 응답 형태로 정확하게 전송한다.
 * 상태 줄 + 필수 헤더 + 빈 줄 + 파일 바디 
 */

void serve_static(int fd, char *filename, int filesize, int is_head){
  int srcfd;
  char buf[MAXBUF], filetype[MAXLINE];
  
  /*filename을 검사하여 MIME 타입을 결정함 */
  get_filetype(filename, filetype);
  /* response line + response header */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));

  /* 빈 파일이거나, head 명령어면 헤더까지만 출력*/
  if(filesize == 0 || is_head == 1) return;

  /* send body */
  const size_t CHUNKSIZE = 64 * 8192; /* 512KB 청크단위*/
  char *srcp = (char *)malloc(CHUNKSIZE);
  if(srcp == NULL){
    unix_error("malloc fail");
    return;
  }
  /* 소켓으로 보내기 위한 정적 콘텐츠 파일 열기 */
  srcfd = Open(filename, O_RDONLY, 0);
  /* 청크사이즈 만큼 메모리(힙) 임시 저장 후 소켓을 통해 전송 */
  /* 총 사이즈를 초기에는 남은 사이즈로 설정*/
  size_t left = (size_t) filesize;

  /* 청크사이즈씩 전송하면서 남김없이 전송할 때까지 반복*/
  while(left > 0){
    /* 청크사이즈보다 작은, 즉 마지막 찌꺼기 인지 확인*/
    size_t want = (left < CHUNKSIZE) ? left : CHUNKSIZE;
    ssize_t nr = Rio_readn(srcfd, srcp, want);
    if(nr <= 0){
      break;
    }
    Rio_writen(fd, srcp, nr);
    /* 보낸 바이트 만큼 남은 바이트에서 빼기 */
    left -= nr;
  }
  close(srcfd);
  free(srcp);
}
void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head){
  
  char buf[MAXBUF];
  char *emptylist[] = { NULL };
  
  /* 응답라인과 헤더 보내기 */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* Fork -> exec 시스템 콜을 통해 자식 프로세스 생성 후 다른 이미지로 교체 */
  if(Fork() == 0){
    /* 환경변수로 전달 */
    setenv("QUERY_STRING", cgiargs,  1);
    setenv("REQUEST_METHOD", is_head ? "HEAD" : "GET", 1);
    Dup2(fd, STDOUT_FILENO); /* 자식 프로세스의 표준 출력을 연결 소켓으로 전달 */
    execve(filename, emptylist, environ);
  }
  wait(NULL);
}

/**
 * 역할: 주어진 filename의 확장자를 바탕으로 적절한 MIME 타입 문자열을 채워넣는다.
 * 6가지 MIME 타입 지원 
 */
void get_filetype(char *filename, char *filetype){
  if(strstr(filename, ".jpg")){
    strcpy(filetype, "image/jpeg");
  } else if(strstr(filename, ".png")){
    strcpy(filetype, "image/png");
  } else if(strstr(filename, ".gif")){
    strcpy(filetype, "image/gif");
  } else if(strstr(filename, ".html")){
    strcpy(filetype, "text/html");
  } else if(strstr(filename, ".mp4")){
    strcpy(filetype, "video/mp4");
  } else {
    strcpy(filetype, "text/plain");
  }
}