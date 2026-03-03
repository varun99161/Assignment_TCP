CC=gcc
CFLAGS=-Wall -O2
LIBS=-lssl -lcrypto

SRC_COMMON = hash.c socket_wrapper.c

all: client server test_hash

client: client.c $(SRC_COMMON)
	$(CC) $(CFLAGS) client.c $(SRC_COMMON) -o client $(LIBS)

server: server.c $(SRC_COMMON)
	$(CC) $(CFLAGS) server.c $(SRC_COMMON) -o server $(LIBS)

test_hash: test_hash.c hash.c
	$(CC) $(CFLAGS) test_hash.c hash.c -o test_hash $(LIBS)

clean:
	rm -f client server test_hash *.o
