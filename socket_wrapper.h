#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

/**
 * @file socket_wrapper.h
 * @brief Minimal cross-platform socket initialization/cleanup interface.
 *
 * This header provides:
 *  - A portable socket handle typedef (`socket_t`) that maps to
 *    `SOCKET` on Windows and `int` on POSIX systems.
 *  - Two functions to initialize and clean up the socket subsystem:
 *      * socket_init()    — WSAStartup on Windows; no-op on POSIX.
 *      * socket_cleanup() — WSACleanup on Windows; no-op on POSIX.
 *
 * Include platform-specific networking headers (e.g., <arpa/inet.h>,
 * <netinet/in.h>) in your .c files where you open/bind/connect sockets.
 */

#ifdef _WIN32
  #include <winsock2.h>   /**< Windows Sockets API header (SOCKET, WSA*). */
  typedef SOCKET socket_t;/**< Portable socket handle on Windows. */
#else
  #include <sys/socket.h> /**< POSIX sockets header (socket(), send(), recv()). */
  typedef int socket_t;   /**< Portable socket handle on POSIX systems. */
#endif

/**
 * @brief Initialize the socket subsystem for the current platform.
 *
 * - **Windows**: Calls `WSAStartup(MAKEWORD(2,2), ...)`.  
 *   Returns 0 on success; non-zero indicates an error code from WinSock.
 * - **POSIX (Linux/macOS)**: No initialization required; always returns 0.
 *
 * @return int 0 on success; non-zero on Windows failure.
 */
int socket_init(void);

/**
 * @brief Clean up the socket subsystem for the current platform.
 *
 * - **Windows**: Calls `WSACleanup()`.
 * - **POSIX (Linux/macOS)**: No cleanup required; does nothing.
 */
void socket_cleanup(void);

#endif /* SOCKET_WRAPPER_H */
