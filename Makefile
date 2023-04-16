sc: src/server.c src/client.c common.o
	cc -O3 src/server.c -o bin/server obj/common.o -pthread
	cc -O3 src/client.c -o bin/client obj/common.o -pthread -lncurses

common.o: src/common.c
	cc -c src/common.c -o obj/common.o