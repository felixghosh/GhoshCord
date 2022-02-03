sc: server.c client.c common.o
	gcc -g server.c -o server common.o -pthread
	gcc -g client.c -o client common.o

common.o: common.c
	gcc -g common.c -o common.o