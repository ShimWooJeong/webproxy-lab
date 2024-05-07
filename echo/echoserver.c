#include "csapp.h"

void echo(int connfd); // 클라이언트와 통신하는 echo 함수 선언

int main(int argc, char **argv)
{
    int listenfd, connfd;                                // 리스닝 소켓 & 연결된 클라이언트 소켓의 파일 디스크립터
    socklen_t clientlen;                                 // 클라이언트 주소 길이
    struct sockaddr_storage clientaddr;                  // 클라이언트의 주소 정보 저장 구조체
    char client_hostname[MAXLINE], client_port[MAXLINE]; // 클라이언트의 호스트 이름 & 포트 번호 저장 배열

    if (argc != 2) // 프로그램 실행 시 포트 번호를 인자로 입력하지 않았을 경우
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]); // 사용법 출력 후 프로그램 종료
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]); // 주어진 포트 번호로 리스닝 소켓 열고 파일 디스크립터 얻어옴
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);                                                  // 클라리언트 주소 길이 초기화
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);                                     // 클라이언트 연결 수락 & 연결된 클라이언트 소켓의 파일 디스크립터 얻음
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); // 클라이언트의 호스트 이름 & 포트 번호 얻음
        printf("Connected to (%s, %s)\n", client_hostname, client_port);                              // 연결된 클라이언트 정보 출력
        echo(connfd);                                                                                 // 클라이언트와 통신하는 echo 함수 호출
        Close(connfd);                                                                                // 클라이언트 소켓 닫음
    }
    exit(0);
}