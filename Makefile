sc: server.c client.c common.o
	cc -g server.c -o server common.o -pthread
	cc -g client.c -o client common.o

common.o: common.c
	cc -g common.c -o common.o