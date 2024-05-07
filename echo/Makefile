CC = gcc
CFLAGS = -O2 -Wall -I .
LIB = -lpthread

all: echoclient echoserver

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c csapp.h

echoclient: echoclient.c csapp.o
	$(CC) $(CFLAGS) -o echoclient echoclient.c csapp.o $(LIB)

echoserver: echoserver.c csapp.o echo.o
	$(CC) $(CFLAGS) -o echoserver echoserver.c csapp.o echo.o $(LIB)

clean:
	rm -f *.o echoclient echoserver *~
	
echo.o: echo.c
	$(CC) $(CFLAGS) -c echo.c

# ./echoserver 1234
# 서버 백그라운드 실행, 포트 번호 1234
# ./echoclient localhost 1234
# 클라이언트 실행 파일 실행, 서버/클라가 같은 컴퓨터에 있으니까 localhost(127.0.0.1) 그리고 서버의 포트 번호 1234 입력
# 그 후 클라이언트에서 입력하면 서버에선 받은 데이터의 바이트를 출력하고, 다시 클라이언트에 똑같은 데이터를 전송