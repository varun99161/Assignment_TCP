/**
 * @file socket_wrapper.c
 * @brief Minimal cross-platform socket subsystem init/cleanup with simple prints.
 *
 * This module provides:
 *   - socket_init(): Initializes the socket subsystem (WSAStartup on Windows,
 *                    no-op on POSIX). Prints a simple status line.
 *   - socket_cleanup(): Cleans up the socket subsystem (WSACleanup on Windows,
 *                       no-op on POSIX). Prints a simple status line.
 *
 * Notes:
 *   - Logging is intentionally minimal and uses printf() only, as requested.
 *   - The includes are limited to those required here.
 */

#include "socket_wrapper.h"
#include <stdio.h>

#ifdef _WIN32
  #include <string.h>  /* For completeness if you later format errors */
  #include <errno.h>
#endif

/**
 * @brief Initialize the socket subsystem for the current platform.
 *
 * Behavior:
 *   - On Windows: calls WSAStartup(MAKEWORD(2,2), ...).
 *       * Prints "socket_init: OK WINDOWS" on success.
 *       * Prints "WSAStartup failed: <code>" on failure.
 *       * Returns the WSAStartup return code (0 on success).
 *   - On POSIX (Linux/macOS): no initialization needed.
 *       * Prints "socket_init: OK LINUX".
 *       * Returns 0.
 *
 * @return int 0 on success; non-zero on Windows failure.
 */
int socket_init() {
#ifdef _WIN32
    WSADATA wsa;
    int ret = WSAStartup(MAKEWORD(2,2), &wsa);
    if (ret != 0) {
        printf("WSAStartup failed: %d\n", ret);
    } else {
        printf("socket_init: OK WINDOWS\n");
    }
    return ret;
#else
    printf("socket_init: OK LINUX\n");
    return 0;
#endif
}

/**
 * @brief Tear down the socket subsystem for the current platform.
 *
 * Behavior:
 *   - On Windows: calls WSACleanup().
 *       * Prints "socket_cleanup: OK WINDOWS" on success.
 *       * Prints "WSACleanup failed: <last_error>" on failure.
 *   - On POSIX (Linux/macOS): no cleanup needed.
 *       * Prints "socket_cleanup: OK LINUX".
 *
 * @return void
 */
void socket_cleanup() {
#ifdef _WIN32
    if (WSACleanup() != 0) {
        printf("WSACleanup failed: %d\n", WSAGetLastError());
    } else {
        printf("socket_cleanup: OK WINDOWS\n");
    }
#else
    printf("socket_cleanup: OK LINUX\n");
#endif
}
