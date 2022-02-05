sc: server.c client.c common.o
	cc -g server.c -o server common.o -pthread
	cc -g client.c -o client common.o -pthread -lncurses
	cc -g window.c -o window -lncurses

common.o: common.c
	cc -c common.c -o common.o