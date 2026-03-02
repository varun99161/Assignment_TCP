
CC=gcc
CFLAGS=-Wall -O2
LIBS=-lssl -lcrypto

all:
	$(CC) src/client.c src/hash.c src/socket_wrapper.c -o client $(LIBS)
	$(CC) src/server.c src/hash.c src/socket_wrapper.c -o server $(LIBS)

clean:
	rm -f client server
