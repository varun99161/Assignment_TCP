
#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
typedef int socket_t;
#endif

int socket_init();
void socket_cleanup();

#endif
