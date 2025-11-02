# CSAPP 10. 시스템 수준 입출력

---

## 1. 입출력이란?

**입출력(I/O)** 는 주 메모리와 디스크, 터미널, 네트워크 같은 외부 장치들 간에 데이터를 복사하는 과정이다. 

- **입력(Input)** : I/O 장치에서 메인 메모리로 데이터 복사
- **출력(Output)** : 데이터를 메인 메모리에서 디바이스로 복사

---

## 2. Unix I/O
> 리눅스에서 파일은 연속된 m개의 바이트다.

- 네트워크, 디스크, 터미널 같은 모든 I/O 디바이스들은 파일로 모델링된다.
- 즉, 모든 I/O는 해당 파일을 읽거나 쓰는 형식으로 수정된다.
- 파일 개념은 모든 입력과 출력이 일정하고 통일된 방식으로 수행되도록 해준다.

### (1) 파일 열기
- APP은 해당 파일을 열겠다고 커널에 요청한다.
- 커널은 이후 **Descriptor**라는 작은 비음수를 리턴한다.
- 커널은 열린 파일에 대한 모든 정보를 모든 정보를 추적한다. 
- APP은 Descriptor만 추적한다.
> 리눅스 쉘이 만든 각 프로세스는 세 개의 열린 파일을 가지고 동작한다.
 - **표준 입력(Standard Input)** : Descriptor 0(`STDIN_FILENO`);
 - **표준 출력(Standard Output)**: Descriptor 1(`STDOUT_FILENO`);
 - **표준 에러(Standard Error)** : Descriptor 2(`STDERR_FILENO`);

### (2) 현재 파일 위치 변경
- 커널은 파일을 열 때마다 파일 위치 K를 관리한다. 
- 파일 위치는 "현재 어디까지 읽었는지 / 썼는지"를 기억하는 변수이다.(일종의 Checkpoint?, 아니면 책갈피?)
- 이것은 처음에 0이지만, 읽거나 쓴 바이트 수만큼 자동으로 증가한다.
- APP은 seek() 호출을 통해 명시적으로 현재 파일의 위치를 설정할 수 있다. 

``` c
int fd = open("jungle.txt", O_RDONLY);

read(fd, buf, 100);

lseek(fd, 50, SEEK_SET);
```

### (3) 파일 읽기 및 쓰기
- **읽기(Read)**: 현재 파일 위치 K에서 시작하여 파일로부터 메모리로 n 바이트를 파일에서 메모리로 복사한다. 단, K가 파일의 크기보다 커지면 **EOF(END-OF-FILE)**가 발동하며, APP이 감지할 수 있다.
- **쓰기(Write)** : 읽기와 마찬가지로 파일 위치 K에 시작하여 메모리에서 파일로 복사한다.

### (4) 파일 닫기
- APP이 파일 접근을 마치면, 즉 더 이상 접근할 필요가 없을 때 커널에 파일을 닫아줄 것을 요청한다.
- 커널은 파일을 열렸을 때 생성했던 자료구조를 반환하고, 해당 Descriptor를 가용 식별자 Pool로 복원한다.
---

## 3. 파일
각각의 리눅스 파일은 시스템에서의 역할을 나타내는 타입을 가진다. 

### (1) 일반 파일
- 임의의 데이터를 포함한다.
- APP은 종종 ASCII문자나 유니코드만을 포함하는 텍스트 파일과 그 외의 모든 파일(바이너리 파일)을 구분한다.
- 그러나 커널은 이 둘을 구분하지 않는다.

### (2) 디렉터리
- 링크들의 배열로 구성된다.
- 각각의 링크는 파일 이름을 파일로 대응시키며, 이 이름은 또 다른 디렉토리일수도 있다.
- 각 디렉토리는 최소 두 개의 항목을 포함한다.  .(점 한개)는 자신의 디렉토리의 링크이며, ..(점 두개)는 부모 디렉토리의 링크이다.

### (3) 소켓
- 네트워크를 통해 다른 프로세스와 통신하는데 사용되는 파일이다.


## 4. 리눅스 디렉토리 계층
- 리눅스 커널은 파일들을 루트 디렉토리(`/`)를 기준으로 모든 파일을 단일 디렉터리 계층으로 구성한다.
- 시스템의 각 파일은, 루트 디렉토리의 직접 또는 간접적인 후손이다.
- 각 프로세스는 현재 작업 디렉토리를 자신의 컨텍스트로 가진다.

### 경로 이름
- 디렉터리 계층구조에서의 위치는 경로 이름으로 명시한다.
- `/home/droh` 기준

- **절대 경로 이름**
    - 루트로부터의 경로
    - `/home/droh/hello.c`

- **상대 경로 이름**
    - 현재 작업 디렉토리로부터의 경로
    - `./hello.c`
---

## 5. 파일 열기와 닫기
- 프로세스는 다음과 같은 `open` 함수를 통해 기존의 파일을 열거나 새 파일을 생성한다.

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(char *filename, int flags, mode_t mode);
// return 열기에 성공하면 해당 파일의 Descriptor 반환, 실패하면 -1 반환

```
- `open` 함수는 filename을 Descriptor로 변환하고 Descriptor 번호를 반환한다.
- 이 때 번호는 해당 프로세스에서 가용 가능한 가장 작은 번호이다.

### (1) Flags 인자
- `O_RDONLY` : 읽기 전용
- `O_WRONLY` : 쓰기 전용
- `O_RDWR`   : 읽기 / 쓰기 가능

flags 인자는 쓰기 작업을 위한 추가 명령을 제공할 수 있도록 비트 마스크을 OR 형태로 제공한다.

- `O_CREAT` : 파일이 존재하면, 빈 파일 생성
- `O_TRUNC` : 파일이 이미 존재하면, 파일의 내용 비우기
- `O_APPEND` : 매 쓰기 연산 전에 파일 위치를 파일의 마지막으로 설정

### (2) mode 인자
mode 인자는 새 파일들의 접근 권한 비트들을 명시한다.

- 각 프로세스는 umask라는 컨텍스트의 일부를 가지며, 이것은 umask 함수를 호출하여 설정한다.
- `open` 함수 호출 시, mode 인자를 사용할 것이다.
-  이를 통해 파일을 생성 시, 그 때 mode & ~umask로 설정된다.

```
#define DEF_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH (666)

#define DEF_UMASK S_IWGRP | S_IWOTH (022)
umask(DEF_UMASK);
fd = open("foo.txt", O_CREAT | O_TRUNC | O_WDONLY, DEF_MODE);
// 666 & ~022 = 644
// Owner는 읽기 / 쓰기, Group은 읽기, Others는 읽기 가능
```

- 프로세스는 오픈한 파일을 close 함수를 호출해서 닫는다.

```c
#include <unistd.h>
int close(int fd);
```

## 6. 파일 읽기와 쓰기

```
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t n);

ssize_t write(int fd, const void *buf, size_t n);
```

### (1) Read 함수
- 식별자 fd의 현재 파일 위치에서 최대 n 바이트를 메모리 위치 buf로 복사한다.
- 리턴 값은 -1은 에러, 0은 EOF를 나타내고, 정상 수행 시 전송된 바이트 수를 나타낸다.

### (2) Write 함수
- 메모리 위치 buf에서 식별자의 현재 파일 위치로 최대 n 바이트를 복사한다.


## 7. RIO 패키지를 이용한 안정적인 읽기와 쓰기
RIO 패키지는 짧은 카운트가 발생할 수 있는 네트워크 프로그램 같은 응용에서 편리하고, 안정적이고 효율적인 I/O를 제공한다. RIO 패키지는 두 가지 종류의 함수를 제공한다. 

- **버퍼 없는 입력 및 출력 함수**
    - 이 함수들은 메모리와 파일 사이에 APP 수준의 버퍼링 없이 직접 전송한다.
    - 네트워크에서 바이너리 데이터를 읽고 쓸 때 유용

- **버퍼를 사용하는 입력 함수**
    - `printf` 같은 표준 입출력 함수와 유사하다.
    - 텍스트 라인들과 내용이 응용 수준 버퍼에 캐시되어 있는 파일의 데이터를 효율적으로 읽을 수 있게 한다.
    - 다른 함수와 달리 RIO 함수들은 Thread-safe하며 같은 식별자에서 임의로 중첩될 수 있다. 


## 8. RIO 버퍼 없는 입력 및 출력 함수
- APP은 `rio_readn` 과 `rio_writen` 함수를 호출하여 메모리와 파일 간에 직접 데이터를 전송할 수 있다.

```c
#include "csapp.h"

// return: 전송 성공 시 전송된 바이트 수, 실패 시 -1, EOF면 0(rio_readn only)
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
```

### rio_readn() 함수
- 식별자 fd의 현재 파일 위치에서 메모리 위치 usrbuf로 최대 n 바이트를 전송한다.
- EOF를 제외하고 짧은 카운트를 반환하지 않는다.

```c
ssize_t rio_readn(int fd, void *usrbuf, size_t n){
    size_t nleft = n; // 앞으로 읽어야 할 바이트 수=
    ssize_t nread;
    char *bufp = usrbuf;

    while(nleft > 0){ // n 바이트를 모두 읽을 때까지 반복
        if((nread = read(fd, bufp, nleft)) < 0){ //오류 반환
            if(errno == EINTR){// 시그널 핸들러에 의한 중단이라면 
                nread = 0; // 다시 재개
            }
            else
                return -1;
        }
        else if(nread == 0) // EOF 도달
            break;
        nleft -= nread; // 읽은 바이트 수를 남은 바이트에서 빼기
        bufp += nread; // 버퍼에 전진
    }
    return (n - nleft); // 실제 총 읽은 바이트 반환(EOF 경우)
}

```

### rio_writen() 함수
- 메모리 위치 usrbuf에서 식별자 fd로 n 바이트를 전송한다.
- 절대 짧은 카운트를 반환하지 않는다.


## 9. RIO 버퍼를 통한 입력 함수
### (1) 버퍼링의 필요성
- 텍스트 파일의 줄 수를 세는 프로그램을 `read` 함수로 1바이트씩 읽는 것은 각 바이트를 읽기 위해 커널이 트랩을 요청해야 한다.
- 더 좋은 방법은 버퍼링 기법으로 텍스트 라인 전체를 내부 읽기 버퍼에서 래퍼 함수(`rio_readlineb`)를 호출하는 것이다.
- 이 방법은 버퍼가 비어있을 때만 `read` 시스템 콜를 자동으로 호출한다.

### (2) 예제
```c
void rio_readinitb(rio_t *rp, int fd);
// fd와 응용 수준 버퍼(rio_t)를 연결하고 내부 상태 초기화

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
// 다음 텍스트 라인(개행 포함)을 usrbuf로 복사하고 마지막에 '\0' 추가
// 최대 maxlen-1 바이트만 담고, 넘치면 잘라서 '\0'로 종결

ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
// 최대 n바이트를 버퍼에서 꺼내서 usrbuf로 복사 (바이너리 안전)
// 원시 바이트를 전송한다.

```

### (3) RIO 버퍼 함수
- `rio_readinitb(rp, fd)`
    ```c
    void rio_readinitb(rio_t *rp, int fd){
        rp -> rio_fd = fd;
        rp -> rio_cnt = 0;
        rp -> rio_bufptr = rp -> rio_buf; /* 내부 읽기 버퍼의 시작 주소*/
    }
    ```
    - open한 식별자마다 한번 호출된다.
    - 이 함수는 식별자 fd는 주소 rp에 위치한 rio_t 타입의 버퍼에 연결한다.
    - 읽기 버퍼를 초기화한다.

- `rio_readlineb(rp, usrbuf, maxlen)`
    - 다음 텍스트 줄을 파일 rp에서(종료 개행 문자 포함) 읽고, 이것을 메모리 위치 usrbuf로 복사한다.
    - 텍스트 라인을 널 문자로 종료시킨다.
    - 최대 maxlen - 1개의 바이트를 읽으며, 종료용 널 문자를 위한 공간을 남겨둔다. 


- `rio_readnb(rp, usrbuf, n)`
    ```c
    ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n){
        size_t nleft = n;
        ssize_t nread;
        char *bufp = usrbuf;

        while(nleft > 0){
            if((nread = rio_read(rp, bufp, nleft)) < 0) // rio_readn가 달리 버퍼라서 rio_read 사용
                return -1
            else if (nread == 0){
                break;
            }
            nleft -= nread;
            bufp += nread;
        }
        return (n - nleft);
    }
    ```
- `rio_read(rp, usrbuf, n)`
    ```c
    static ssize_t rio_read(rio_t *rp, void *usrbuf, size_t n){
        int cnt;

        while(rp -> rio_cnt <= 0){ // 버퍼가 비어 있으면 채우기(read())
            rp -> rio_cnt = read(rp -> rio_fd, rp -> rio_buf, sizeof(rp -> rio_buf)); // 한꺼번에 버퍼로 가져오기
            if(rp -> rio_cnt < 0){  
                if(errno != EINTR) // 시그널 핸들러에 의해 끊긴게 아니면 진짜 에러임
                    return -1;
            } else if(rp -> rio_cnt == 0){
                return 0; // EOF : 더 이상 읽을 게 없으면 종료
            } else {
                rp -> rio_bufptr = rp -> rio_buf; //새로 채웠으니 다시 리셋
            }
        }

        cnt = n;
        if(rp -> rio_cnt < n)
            cnt = rp -> rio_cnt;
        memcpy(usrbuf, rp -> rio_bufptr, cnt);
        rp -> rio_bufptr += cnt; // 저장한 만큼 포인터 전진
        rp -> rio_cnt -= cnt;   // 실제 저장한 양 제거
        return cnt;
    }
    ```
    - RIO 읽기 루틴의 핵심이다.
    - rio_read가 n 바이트를 읽기 위한 요청으로 호출되면, rp -> rio_cnt개의 읽지 않은 바이트가 읽기 버퍼 내에 존재한다.
    - 텍스트 라인을 널 문자로 종료시킨다.
    - 최대 maxlen - 1개의 바이트를 읽으며, 종료용 널 문자를 위한 공간을 남겨둔다. 

### (4) 주의사항
- **혼용(interleaving)** : 버퍼를 사용하는 RIO 함수들은 Thread-safe하며 같은 식별자에서 임의로 중첩될 수 있다.


## 10. 파일 메타데이터 읽기
```c
#include <unistd.h>
#include <sys/stat.h>

int stat(const char *filename, struct stat *buf);
int fstat(int fd, struct stat *buf);
```

- `stat` 함수는 **파일 이름**을 입력으로 받아 stat 구조체의 멤버들을 채워준다.
- `fstat` 함수는 `stat`와 유사히자만, 파일 이름 대신 **파일 식별자**를 사용한다.

### (1) struct `stat` 구조체

``` c
/* Metadata returned by the stat and fstat functions */
struct stat {
    dev_t st_dev; /* Device */
    ino_t st_ino; /* inode */
    mode_t st_mode; /* Protection and file type */
    nlink_t st_nlink; /* Number of hard links uid_t st_uid; /* User ID of owner */
    gid_t st_gid; /* Group ID of owner */
    dev_t st_rdev; /* Device type (if inode device) */
    off_t st_size; /* Total size, in bytes unsigned long st_blksize; /* Block size for filesystem I/O */
    unsigned long st_blocks; /* Number of blocks allocated */
    time_t st_atime; /* Time of last access */
    time_t st_mtime; /* Time of last modification */
    time_t st_ctime; /* Time of last change */
};
```
- `st_size` : 파일 크기를 바이트로 저장한 멤머
- `st_mode` : 파일 권한 비트와 파일 타입 모두를 인코딩한다.

### (2) 파일의 st_mode를 통한 파일 타입 확인하기
- `S_ISREG(m)` : 일반 파일인가?
- `S_ISDIR(m)` : 디렉터리인가?
- `S_ISSOCK(m)`: 네트워크 소켓인가?

## 11. 디렉터리 내용 읽기
APP은 `readdir` 계열의 함수를 이용해 디렉터리의 내용을 읽을 수 있다.

### (1) `opendir` 함수
```c
#include <sys/types.h>
#include <dirent.h>

DIR *opendir(const char *name);
```
- `opendir` 함수는 경로 이름을 받아서 디렉토리 스트림을 가리키는 포인터를 리턴한다.
- 스트림은 항목들의 정렬된 리스트에 대한 추상화이다.

### (2) `readdir` 함수
```
#include <dirent.h>
struct dirent *readdir(DIR *dirp);
```
- `readdir`를 호출할 경우 dirp 스트림에서 다음 디렉토리 항목으로의 포인터를 리턴하거나 더 이상 항목이 없으면 NULL을 리턴한다.
- 각 디렉터리 항목은 다음과 같은 `struct dirent` 구조체이다.

```
struct dirent {
    ino_t d_ino;   // 파일 위치(아이노드)
    char d_name[256]; // 파일의 이름
}
```


### (3) `closedir` 함수
```
#include <dirent.h>
int closedir(DIR *dirp);
```
- `closedir` 함수는 스트림을 닫고 관련 리소스를 해제한다.

### (4) 예제 
```c
#include "csapp.h"
int main(int argc, char **argv){
    DIR *streamp; /* 디렉토리 스트림을 가리키는 포인터*/
    struct dirent *dep; // 디렉토리 엔트리

    streamp = Opendir(argv[1]);

    errno = 0;
    while((dep = readdir(streamp)) != NULL){
        printf("Found file : %s\n", dep -> d_name);
    }
    if (errno != 0){
        unix_error("readdir error");
    }
    Closedir(streamp);
    exit(0);
}

```
## 12. 파일 공유
리눅스 파일은 여러 가지 방법으로 공유될 수 있다.

1. **Descriptor Table**
    - 각 프로세스는 자신만의 별도의 식별자 테이블을 가진다.
    - 각 Entry는 프로세스의 오픈된 파일 식별자로 인덱스된다.
    - 식별자 번호 -> 파일 테이블 Entry로 Mapping
    - 프로세스가 파일을 다룰 때 사용하는 간단한 정수를 제공한다.

2. **File Table**
    - 오픈 파일들은 모든 프로세스에 의해 공유하는 한 개의 파일 테이블로 표시된다.
    - 파일 테이블은 여러 개의 파일 테이블 Entry를 가진다.
    - 각 파일 테이블 Entry가 포함하는 것
        - 현재 파일 위치
        - 현재 가리키고 있는 식별자 Entry들의 참조 횟수
        - v-노드 테이블 Entry의 포인터
    - Descriptor를 닫으면 참조 횟수가 1씩 감소한다. 참조 횟수가 0이 되면 지운다.
    - 즉, Descriptor 번호가 달라도 가리키는 엔트리가 같다면 같은 파일이라고 생각하자.

3. **v-node table**
    - 파일 테이블처럼 v-노드 테이블은 모든 프로세스들이 공유하고 있다.
    - 각 엔트리는 실제 파일의 메타데이터인 `stat` 구조체의 대부분 정보를 가지고 있다.
    - 파일 시스템에 존재하는 파일 그 자체를 커널 메모리 상에 표현한다.
    - 파일 유형, 권한, 크기, 소유자 등 stat 구조체에 담기는 모든 메타데이터의 원본을 저장한다.
















### 📂 다운로드 후 폴더 구조 설명

```
webproxy_lab_docker/
├── .devcontainer/
│   ├── devcontainer.json      # VSCode에서 컨테이너 환경 설정
│   └── Dockerfile             # C 개발 환경 이미지 정의
│
├── .vscode/
│   ├── launch.json            # 디버깅 설정 (F5 실행용)
│   └── tasks.json             # 컴파일 자동화 설정
│
├── webproxy-lab
│   ├── tidy                    # tidy 웹 서버 구현 폴더
│   │  ├── cgi-bin              # tidy 웹 서버를 테스트하기 위한 동적 컨텐츠를 구현하기 위한 폴더
│   │  ├── home.html            # tidy 웹 서버를 테스트하기 위한 정적 HTML 파일
│   │  ├── tidy.c               # tidy 웹 서버 구현 파일
│   │  └── Makefile             # tidy 웹 서버를 컴파일하기 위한 파일
│   ├── Makefile                # proxy 웹 서버를 컴파일하기 위한 파일
│   └── proxy.c                 # proxy 웹 서버 구현 파일
│
└── README.md  # 설치 및 사용법 설명 문서
```
---

## 5. VSCode에서 해당 프로젝트 폴더 열기

1. VSCode를 실행
2. `파일 → 폴더 열기`로 방금 클론한 `webproxy_lab_docker` 폴더를 선택

---

## 6. 개발 컨테이너: 컨테이너에서 열기

1. VSCode에서 `Ctrl+Shift+P` (Windows/Linux) 또는 `Cmd+Shift+P` (macOS)를 누릅니다.
2. 명령어 팔레트에서 `Dev Containers: Reopen in Container`를 선택합니다.
3. 이후 컨테이너가 자동으로 실행되고 빌드됩니다. 처음 컨테이너를 열면 빌드하는 시간이 오래걸릴 수 있습니다. 빌드 후, 프로젝트가 **컨테이너 안에서 실행됨**.

---

## 7. C 파일에 브레이크포인트 설정 후 디버깅 (F5)

이제 본격적으로 문제를 풀 시간입니다. `webproxy-lab/README.md` 파일을 참조하셔서 webproxy 문제를 풀어보세요.
구현 순서는 tidy 웹서버(`webproxy-lab/tidy/tidy.c`)를 CSApp책에 있는 코드를 이용해서 구현하고, proxy서버(`webproxy-lab/proxy.c`)를 구현한 뒤에 최종 `webproxy-lab/mdriver`를 실행하여 70점 만점을 목표로 구현하세요.

C 언어로 문제를 풀다가 디버깅이 필요하시면 소스코드에 BreakPoint를 설정한 뒤에 키보드에서 `F5`를 눌러 디버깅을 시작할 수 있습니다. 디버깅은 tidy 서버와 proxy 서버용 2가지로 제공되며 각각 "Debug Tidy Server", "Debug Proxy Server" 이름을 가집니다. 두가지 중 원하는 디버깅 설정을 선택한 뒤에 `F5`를 누르면 해당 서버가 디버깅모드로 실행됩니다. 

* 기본적으로 "Debug Tidy Server"는 tidy 서버를 실행할때 포트를 `8000`을, "Debug Proxy Server"는 `4500`를  사용합니다. 해당 포트를 이미 다른 프로세스가 사용중이라면 새로운 포트로(`launch.json`파일에서 가능) 변경한 뒤에 디버깅을 진행합니다.


---

## 8. 새로운 Git 리포지토리에 Commit & Push 하기

금주 프로젝트를 개인 Git 리포와 같은 다른 리포지토리에 업로드하려면, 기존 Git 연결을 제거하고 새롭게 초기화해야 합니다.

### ✅ 완전히 새로운 Git 리포로 업로드하는 방법

아래 명령어를 순서대로 실행하세요:

```bash
rm -rf .git
git init
git remote add origin https://github.com/myusername/my-new-repo.git
git add .
git commit -m "Clean start"
git push -u origin main
```

### 📌 설명

- `rm -rf .git`: 기존 Git 기록과 연결을 완전히 삭제합니다.
- `git init`: 현재 폴더를 새로운 Git 리포지토리로 초기화합니다.
- `git remote add origin ...`: 새로운 리포지토리 주소를 origin으로 등록합니다.
- `git add .` 및 `git commit`: 모든 파일을 커밋합니다.
- `git push`: 새로운 리포에 최초 업로드(Push)합니다.

이 과정을 거치면 기존 리포와의 연결은 완전히 제거되고, **새로운 독립적인 프로젝트로 관리**할 수 있습니다.



### ✅ AWS EC2와의 차이점

| 구분 | EC2 같은 VM | Docker 컨테이너 |
|------|-------------|-----------------|
| 실행 단위 | OS 포함 전체 | 애플리케이션 단위 |
| 실행 속도 | 느림 (수십 초 이상) | 매우 빠름 (거의 즉시) |
| 리소스 사용 | 무거움 | 가벼움 |
