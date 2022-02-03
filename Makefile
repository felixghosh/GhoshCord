CC=gcc
CFLAGS=-g
BINS=server

all: $(BINS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $^