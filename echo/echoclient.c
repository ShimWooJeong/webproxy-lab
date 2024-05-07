#include "csapp.h"

// 0번 째 인자: 실행 파일, 1번 째 인자: 호스트 네임, 2번 째 인자: 포트 번호
int main(int argc, char **argv) // argc: 입력받은 인자 수
{                               // argv: 입력받은 인자 배열
    int clientfd;
    char *host, *port, buf[MAXLINE]; // MAXLINE: 버퍼의 최대 크기
    rio_t rio;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1]; // 입력받은 첫 번째 인자를 host에 저장
    port = argv[2]; // 입력받은 두 번째 인자를 port에 저장

    clientfd = Open_clientfd(host, port); // open_clientfd 함수를 호출해 서버와 연결 & 반환받은 소켓 식별자를 clientfd에 저장
    Rio_readinitb(&rio, clientfd);        // rio 구조체 초기화 & 읽기 버퍼 rio를 통해 파일 디스크립터 clientfd에 대한 읽기 작업을 수행할 수 있도록 연결

    // 반복해서 유저에게 받은 입력을 buf에 저장
    while (Fgets(buf, MAXLINE, stdin) != NULL) // 입력이 끊기거나, 오류 발생 시 반복문 종료
    {
        Rio_writen(clientfd, buf, strlen(buf)); // 파일 디스크립터를 통해 buf에 저장된 데이터를 서버로 전송
        Rio_readlineb(&rio, buf, MAXLINE);      // rio 구조체를 통해 파일 디스크립터에서 한 줄의 문자열을 읽어와 buf에 저장
        Fputs(buf, stdout);                     // buf에 저장된 문자열을 표준 출력 stdout에 출력
    }
    Close(clientfd); // 파일 디스크립터를 닫음으로써 클라이언트의 연결 종료 & 사용한 리소스 반환
    exit(0);
}