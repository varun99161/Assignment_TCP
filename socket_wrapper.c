
#include "socket_wrapper.h"

int socket_init() {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2,2), &wsa);
#else
    return 0;
#endif
}

void socket_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}
