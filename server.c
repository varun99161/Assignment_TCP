/**
 * @file server.c
 * @brief Simple file-transfer server with basic logging and retry support.
 *
 * The server:
 *   - Listens on a TCP port provided via argv[1].
 *   - Receives fixed-size "chunks" preceded by a header (packet_header_t).
 *   - Validates each chunk with SHA-256 (provided by hash.h).
 *   - Writes valid chunks to OUTPUT_FILE and sends ACK.
 *   - On hash mismatch or invalid packet size, sends RETRY.
 *   - Stops when it receives a header with FLAG_END.
 *
 * Logging:
 *   - Very simple: appends lines to server.log using logmsg().
 *   - Also prints some user-friendly messages to stdout.
 */

#include "protocol.h"
#include "hash.h"
#include "socket_wrapper.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_RETRIES   5
#define LOG_FILE_PATH "server.log"
#define OUTPUT_FILE   "received_file.bin"

/* Global log handle (basic file logging) */
FILE *log_fp = NULL;

/**
 * @brief Append a single line to the simple log file.
 *
 * @param msg  Null-terminated message string. A newline is added automatically.
 *
 * Note: No timestamps/levels—kept intentionally simple.
 */
void logmsg(const char *msg) {
    if (log_fp) {
        fprintf(log_fp, "%s\n", msg);
        fflush(log_fp);
    }
}

/**
 * @brief Receive exactly @p len bytes into @p buf (blocking loop).
 *
 * @param sock  Connected socket descriptor.
 * @param buf   Destination buffer.
 * @param len   Number of bytes to receive.
 * @return ssize_t  Total bytes received (== len on success),
 *                  0 on peer close,
 *                 -1 or <=0 on error/interrupt per recv().
 *
 * This helper loops until the requested length is received or an error occurs.
 */
ssize_t recv_all(int sock, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = recv(sock, (char*)buf + total, len - total, 0);
        if (n <= 0) return n;      // 0: peer closed; <0: error
        total += (size_t)n;
    }
    return (ssize_t)total;
}

/**
 * @brief Send exactly @p len bytes from @p buf (blocking loop).
 *
 * @param sock  Connected socket descriptor.
 * @param buf   Source buffer.
 * @param len   Number of bytes to send.
 * @return ssize_t  Total bytes sent (== len on success), <=0 on error per send().
 *
 * This helper loops until the requested length is sent or an error occurs.
 */
ssize_t send_all(int sock, const void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(sock, (const char*)buf + total, len - total, 0);
        if (n <= 0) return n;      // <0: error
        total += (size_t)n;
    }
    return (ssize_t)total;
}

/**
 * @brief Program entry point. Starts a TCP server, receives chunks, validates with SHA-256, and writes to file.
 *
 * @param argc  Argument count. Requires at least 2 (program name + port).
 * @param argv  Argument vector. argv[1] must be the port number.
 * @return int  0 on normal termination; non-zero on setup/runtime errors.
 *
 * Flow:
 *   1) Open log file, initialize socket layer.
 *   2) Create/bind/listen on server socket.
 *   3) Accept one client.
 *   4) Loop:
 *        - Read header.
 *        - If FLAG_END: break.
 *        - Validate payload_size.
 *        - Read payload.
 *        - Compute SHA-256 and compare with header.hash.
 *        - On match: write to file, send ACK.
 *        - On mismatch/invalid: send RETRY (limit retries).
 *   5) Clean up and exit.
 */
int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    /* Open very simple log file */
    log_fp = fopen(LOG_FILE_PATH, "a");
    if (!log_fp) {
        printf("Failed to open log file\n");
        return 1;
    }

    /* Initialize platform socket layer (no-op on POSIX, WSAStartup on Windows) */
    socket_init();

    int port = atoi(argv[1]);

    /* Create a TCP socket */
    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);

    /* Bind to 0.0.0.0:<port> */
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* Minimal error handling (kept simple by request) */
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    printf("Server listening on port %d\n", port);
    logmsg("Server started");

    /* Accept a single client */
    socket_t client = accept(server_fd, NULL, NULL);
    if (client <= 0) {
        logmsg("Accept failed");
        fclose(log_fp);
        return 1;
    }
    logmsg("Client connected");

    /* Open destination file for received data */
    FILE *fp = fopen(OUTPUT_FILE, "wb");
    if (!fp) {
        printf("Failed to open output file\n");
        logmsg("Failed to open output file");
        fclose(log_fp);
        return 1;
    }

    packet_header_t header;     /* Protocol header with payload_size/flags/hash */
    uint8_t buffer[CHUNK_SIZE]; /* Payload buffer; CHUNK_SIZE defined in protocol.h */
    uint8_t calc_hash[32];      /* Computed SHA-256 for current chunk */

    int retry_count = 0;        /* Counts consecutive retries for a chunk */

    while (1) {

        /* Receive the fixed-size header for the next packet */
        if (recv_all(client, &header, sizeof(header)) <= 0) {
            printf("Client disconnected\n");
            logmsg("Client disconnected while receiving header");
            break;
        }

        /* If client indicates end-of-transfer, finish up */
        if (header.flags & FLAG_END) {
            printf("Transfer complete\n");
            logmsg("Transfer complete");
            break;
        }

        /* Basic sanity check on payload length */
        if (header.payload_size > CHUNK_SIZE) {
            printf("Invalid packet size\n");
            logmsg("Corrupt packet: invalid size");
            header.flags = FLAG_RETRY;                  /* Ask client to resend */
            send_all(client, &header, sizeof(header));
            continue;
        }

        /* Read the payload bytes that follow the header */
        if (recv_all(client, buffer, header.payload_size) <= 0) {
            printf("Receive failed\n");
            logmsg("Payload receive failure");
            break;
        }

        /* Compute SHA-256 of the received payload and compare to header.hash */
        sha256(buffer, header.payload_size, calc_hash);

        if (memcmp(calc_hash, header.hash, 32) == 0) {
            /* Hash OK: write to file, acknowledge */
            fwrite(buffer, 1, header.payload_size, fp);
            header.flags = FLAG_ACK;
            send_all(client, &header, sizeof(header));

            retry_count = 0;               /* Reset retry counter on success */
            logmsg("Chunk OK, ACK sent");

        } else {
            /* Hash mismatch: request retry and enforce a simple retry limit */
            header.flags = FLAG_RETRY;
            send_all(client, &header, sizeof(header));

            retry_count++;
            logmsg("Hash mismatch -> RETRY sent");

            if (retry_count >= MAX_RETRIES) {
                logmsg("Max retries exceeded, aborting.");
                printf("Too many retry attempts\n");
                break;
            }
        }
    }

    /* Cleanup resources (kept minimal) */
    fclose(fp);
    fclose(log_fp);
    socket_cleanup();
    return 0;
}
