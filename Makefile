
CC=gcc
CFLAGS=-Wall -O2
LIBS=-lssl -lcrypto

all:
	$(CC) client.c hash.c socket_wrapper.c -o client $(LIBS)
	$(CC) server.c hash.c socket_wrapper.c -o server $(LIBS)

clean:
	rm -f client server
