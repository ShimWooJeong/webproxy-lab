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
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

// 입력: ./tiny 8000 -> argc=2, argv[0]=tiny(실행파일), argv[1]=8000(포트 번호)
int main(int argc, char **argv) // argc: 입력받은 인자 수
{                               // argv: 입력받은 인자 배열
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
  // 입력받은 포트 번호로 open_listenfd 실행
  // open_listenfd는 위 포트 번호에 연결 요청을 받을 준비가 된 듣기 식별자 반환(listenfd)
  listenfd = Open_listenfd(argv[1]);
  // 서버 프로세스 종료 전까지 무한 대기
  while (1)
  {
    // accept 함수에 인자로 넣을 주소 길이 계산
    clientlen = sizeof(clientaddr);
    // 반복적으로 연결 요청 접수
    // accept는 1. 듣기 식별자, 2. 소켓 주소 구조체(clientaddr)의 주소, 3. 주소 길이를 인자로 받음
    // accept로 클라이언트 연결 수락 & 연결된 클라이언트 소켓의 파일 디스크립터 반환받아 connfd 생성
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept

    // Getaddrinfo는 (호스트 이름: 호스트 주소, 서비스 이름: 포트 번호)를 소켓 주소 구조체로 변환
    // Getnameinfo는 소켓 주소 구조체에서 스트링 표시로 hostname, port 변환
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port); // 어떤 클라이언트가 들어왔는지, hostname&port 출력
    // 트랜잭션 수행
    doit(connfd); // line:netp:tiny:doit
    // 트랙잭션 수행 후 자신 쪽의 연결 끝(소켓)을 close -> while문 돌기
    Close(connfd); // line:netp:tiny:close
  }
}

// doit(): 한 개의 HTTP 트랜잭션 처리, 입력 인자 fd=connfd
// 클라이언트의 요청 라인 확인 -> 정적/동적 콘텐츠 확인 후 돌려줌
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio; // rio_t 타입의 읽기 버퍼 선언

  // 무한루프 수정: 요청을 받아오지 못했다면 바로 return하여 doit을 종료
  Rio_readinitb(&rio, fd);                  // fd(connfd)로 rio_t 구조체 초기화
  if (!(Rio_readlineb(&rio, buf, MAXLINE))) // 요청 라인 읽어들이고, buf에 저장
  {
    return;
  }

  /*
  rio_readlineb(): 텍스트 줄을 파일 rio에서부터 읽어와 메모리 위치 buf로 복사 -> 읽은 텍스트 라인을 null문자로 바꾸고 종료
  최대 MAXNINE-1개의 바이트를 읽고, 그걸 넘는 텍스트 라인들은 잘라서 null 문자로 종료
  MAXLINE 전에 '\n' 개행 문자를 만날 경우 break;
  이는 rio_writen을 통해 서버면 클라이언트, 클라이언트면 서버에서 보내진 정보를 읽을 때 사용함
  */

  printf("Request headers:\n");
  printf("%s", buf);
  // sscanf?: buf에서 데이터를 "%s %s %s" 형식에 따라 읽어와 각각 method, uri, version에 저장
  sscanf(buf, "%s %s %s", method, uri, version);
  // Tiny는 GET 메소드만 지원하기에 클라이언트가 다른 메소드(POST 등)을 요청하면 error 메세지를 보내고 main으로 돌아옴
  // strcasecmp(): 대소문자 무시하는 문자열 비교 함수
  if (strcasecmp(method, "GET"))
  {
    // GET 메소드가 아니라면, tiny에서 지원하지 않는 메소드이므로 error 출력 후 return
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  printf("-----------------------\n");
  // 요청 헤더 읽기
  read_requesthdrs(&rio);
  printf("-----------------------\n");
  // uri로부터 filename과 cgiargs에 값을 넣는 parse_uri 실행
  // 실행 후 정적/동적 여부에 따라 return 받은 1 or 0을 is_static에 대입
  // 즉 is_static = 정적/동적 컨텐츠를 위한 것인지 나타내는 플래그
  is_static = parse_uri(uri, filename, cgiargs);

  // 만약 파일이 디스크 상에 있지 않다면
  if (stat(filename, &sbuf) < 0)
  {
    // 클라이언트에게 error 메세지를 보내고 main으로 return
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static)
  {
    // 정적 컨텐츠 & 보통 파일(S_ISREG) & 읽기 권한(S_IRUSR)을 가졌는지 검증
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read this file");
      return;
    }
    // 위 조건을 다 만족한다면 -> 정적 컨텐츠를 클라이언트에게 제공
    serve_static(fd, filename, sbuf.st_size); // method 인자 추가=과제?
  }
  else
  {
    // 동적 컨텐츠 & 보통 파일(S_ISREG) & 실행 가능 파일(S_IXUSR)인 지 검증
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      printf("line 160\n");
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read this file");
      return;
    }
    // 위 조건을 다 만족한다면 -> 동적 컨텐츠를 클라이언트에게 제공
    serve_dynamic(fd, filename, cgiargs); // method 인자 추가=과제?
  }
}

// 클라이언트에게 에러 메세지를 보내는 함수
// HTTP 응답을 적절한 상태 코드/메세지와 함께 클라이언트에게 보냄
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /*
  HTML 응답은 본체에서 컨텐츠의 크기와 타입을 나타내야 하기에, HTML 컨텐츠를 한 개의 스트링으로 만듬
  이는 sprintf를 통해 body는 인자에 스택되어 하나의 긴 스트링이 저장된다
  sprintf?: 출력하는 결과 값을 변수에 저장하게 해주는 기능
  */

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);
  // body배열에 차곡차곡 html을 쌓아 넣어줌

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);

  // rio_writen은 서버->클라이언트, 클라이언트->서버로 데이터를 전송할 때 사용
  // rio_writen(): buf에서 식별자 fd로 n바이트만큼 전송
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  // buf에 넣고/보내기 반복

  // sprintf로 쌓아놓은 길쭉한 배열(body, buf)를 fd로 보내줌
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

// Tiny는 요청 헤더 내 어떤 정보도 사용하지 않음
// 단순히 이들을 읽고 무시함
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf); // 이거 없으면 host 정보 빠짐
  /*
  strcmp(): 두 문자열 비교 함수
  헤더의 마지막 줄은 비어있기에 \r\n만 buf에 담겨있다면? = while문 탈출
  buf == '\r\n'이 되는 경우는 모든 줄을 읽고 마지막 줄에 도착한 경우임
  */
  while (strcmp(buf, "\r\n"))
  {
    // rio_readlineb는 '\n'을 만날 때 멈춤
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    // 멈춘 지점까지 출력 후 다시 반복문
  }
  return;
}

// HTTP URI 분석
// URI를 파일 이름 & CGI 인자 스트링으로 분석 -> 요청이 정적/동적 컨텐츠를 위한 것인지 나타내는 플래그 설정
// Tiny는 정적 컨텐츠를 위한 홈 디렉토리가 자신의 현재 디렉토리,
// 실행파일의 홈 디렉토리는 /cgi-bin이라고 가정
// 스트링 cgi-bin을 포함하는 모든 URI는 동적 컨텐츠를 요청하는 것을 나타낸다고 가정
// URI 예시
// static(정적):어쩌구 저쩌구,/,/adder.html
// dynamic(동적): /cgi-bin/adder?first=1213&second=1232
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // strstr()로 uri에 'cgi-bin'이 들어있는지 확인하고 양수값을 리턴하면 동적 컨텐츠를 요구하는 것 -> 조건문 탈출
  // strstr(대상 문자열, 검색할 문자열): 문자열 안에서 문자열 검색 -> 찾으면 문자열의 포인터 반환, 없으면 NULL
  if (!strstr(uri, "cgi-bin")) // 없으면 NULL 반환 = static content
  {
    strcpy(cgiargs, "");   // cgiargs 인자 string 지움
    strcpy(filename, "."); // uri를 상대 리눅스 경로 이름으로 변환
    strcat(filename, uri); // 문자열을 붙임 strcat(최종 문자열, 붙일 문자열)
    // 결과) cgiargs = "", filename="./~~" or "./home.html"

    // 만약 uri가 '/' 문자로 끝난다면 기본 파일 이름(home.html)을 추가
    if (uri[strlen(uri) - 1] == '/')
    {
      strcat(filename, "home.html"); // home.html
    }
    return 1;
  }
  // 있으면 양수값 반환 = 동적 dynamic content
  else
  {
    // index 함수는 문자열에서 특정 문자의 위치 반환
    ptr = index(uri, '?');

    // ? 존재한다면
    if (ptr)
    {
      // 인자로 주어진 값들을 cgiargs 변수에 넣음
      strcpy(cgiargs, ptr + 1); //? 뒤부터 끝까지('\0'을 만날 때까지) 넣음
      // uri에서 방금 복사한 값 제거
      *ptr = '\0';
    }
    // ? 존재하지 않는다면
    else
    {
      // 빈칸으로 두고
      strcpy(cgiargs, "");
    }
    // 나머지 uri 부분을 상대 리눅스 파일 이름으로 변환
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype); // 어떤 파일형식인지 검사
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf); // while을 한번돌면 close가 되고, 새로 연결하더라도 새로 connect하므로 close가 default가됨
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); // \r\n 빈줄 하나가 헤더종료표시

  // rio_writen = client쪽 buf에서 strlen(buf)바이트만큼 fd로 전송
  Rio_writen(fd, buf, strlen(buf));

  // 서버 쪽 출력
  printf("Response headers:\n");
  printf("%s", buf);

  // unix I/O에서 open(): 열려고 하는 파일의 식별자 번호 반환
  // open(열려고 하는 대상 파일 이름, 파일 열 때 적용되는 옵셥, 파일 열 때 접근 권한 설명) -> return 파일 디스크립터
  // O_RDONLY: 읽기 전용으로 파일열기 -> 즉 filename 파일을 읽기 전용으로 열어 식별자 받아옴
  srcfd = Open(filename, O_RDONLY, 0);

  // mmap()로 바로 메모리 할당, srcfd의 파일 값 배정
  // mmap(): malloc과 유사 + 값 복사해줌
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  // 생성한 파일 식별자 번호인 srcfd 닫음
  Close(srcfd);
  // rio_writen()로 클라이언트에 전송
  Rio_writen(fd, srcp, filesize);
  // 할당된 가상 메모리 해제(=free), srcp 주소부터 filesize만큼
  Munmap(srcp, filesize);
}

// HTML 서버가 처리할 수 있는 파일 타입을 이 함수로 제공
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

// 동적 컨텐츠를 클라이언트에게 제공
// HTTP 요청을 받아 CGI 프로그램 실행 -> CGI 프로그램 출력을 클라이언트에게 전달
// -> 동적 컨텐츠를 생성하고 웹 서버 동작 가능
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL}; // emprylist: 포인터 배열

  // 클라이언트에게 성공 알림
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // fork(): 현재 프로세스의 복제본 생성, 부모/자식 프로세스 별도로 동시에 실행시킴
  if (Fork() == 0)
  {
    // 부모 프로세스는 자식의 PID, 자식 프로세스는 0 반환 받음
    // QUERY_STRING 환경변수를 요청 uri의 cgi 인자들을 넣음
    // 세 번째 인자->기존 환경 변수의 유무에 상관없이 값 변경: 1, 아니라면 0
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO); // 자식 프로세스의 표준 출력을 연결 파일 식별자로 재지정
    // CGI 프로그램이 표준 출력으로 쓰는 모든 것은 클라이언트로 바로 감 (부모 프로세스 간섭 x)
    Execve(filename, emptylist, environ);
    // 이후 cgi 프로그램을 로드&실행
    // 프로그램의 출력은 바로 클라이언트로 감
  }
  // 부모 프로세스가 자식 프로세스 종료될 때까지 대기
  Wait(NULL);
}