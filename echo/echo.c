#include "csapp.h"

void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd); // 읽기 버퍼 rio와 서버의 연결 소켓 식별자 연결

    // 읽기 버퍼 rio에서 클라이언트가 보낸 데이터를 읽고, rio에 그 데이터를 그대로 저장
    // 그리고 그 rio에서 MAXLINE만큼의 데이터를 읽어와 buf에 넣음
    while ((n = rio_readlineb(&rio, buf, MAXLINE)) != 0) // 0은 클라이언트 식별자가 닫힘을 의미
    {
        printf("server received %d bytes\n", (int)n);

        // buf에는 클라이언트가 보낸 데이터가 그대로 있음
        // buf 메모리 안에 있는 클라이언트가 보낸 바이트 만큼(=전체)을 connfd로 보냄
        Rio_writen(connfd, buf, n);
    }
}