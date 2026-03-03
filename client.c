/**
 * @file client.c
 * @brief Simple file-transfer client with basic logging and retry support.
 *
 * The client:
 *   - Connects to a server at <server_ip>:<port>.
 *   - Reads the input file in CHUNK_SIZE blocks.
 *   - For each chunk: computes SHA-256, sends header + payload.
 *   - Waits for server response: ACK (advance) or RETRY (resend same chunk).
 *   - Sends a final header with FLAG_END upon EOF to signal completion.
 *
 * Logging:
 *   - Very simple: appends lines to client.log via logmsg().
 *   - Prints user-friendly messages to stdout on key events/errors.
 */

#include "protocol.h"
#include "hash.h"
#include "socket_wrapper.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

#define MAX_RETRIES   5
#define LOG_FILE_PATH "client.log"

/* Global log handle (basic file logging) */
FILE *log_fp = NULL;

/**
 * @brief Append a single line to a simple log file (no timestamps/levels).
 *
 * @param msg  Null-terminated message string. A newline is appended.
 */
static void logmsg(const char *msg) {
    if (log_fp) {
        fprintf(log_fp, "%s\n", msg);
        fflush(log_fp);
    }
}

/**
 * @brief Receive exactly @p len bytes into @p buf, looping until complete or error.
 *
 * @param sock  Connected socket descriptor.
 * @param buf   Destination buffer.
 * @param len   Number of bytes to receive.
 * @return ssize_t  Total bytes received (== len on success),
 *                  0 on peer close,
 *                 <0 on error (per recv()).
 */
static ssize_t recv_all(int sock, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = recv(sock, (char*)buf + total, len - total, 0);
        if (n <= 0) return n;  // 0: peer closed, <0: error
        total += (size_t)n;
    }
    return (ssize_t)total;
}

/**
 * @brief Send exactly @p len bytes from @p buf, looping until complete or error.
 *
 * @param sock  Connected socket descriptor.
 * @param buf   Source buffer.
 * @param len   Number of bytes to send.
 * @return ssize_t  Total bytes sent (== len on success), <=0 on error (per send()).
 */
static ssize_t send_all(int sock, const void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(sock, (const char*)buf + total, len - total, 0);
        if (n <= 0) return n;  // <0: error
        total += (size_t)n;
    }
    return (ssize_t)total;
}

/**
 * @brief Program entry point. Connects to the server and transmits a file in chunks.
 *
 * @param argc  Argument count. Requires 4: program, server_ip, port, input_file.
 * @param argv  Argument vector.
 *              - argv[1]: server IP (e.g., "127.0.0.1")
 *              - argv[2]: server port (e.g., "9000")
 *              - argv[3]: path to input file to send
 * @return int  0 on success, non-zero on error (socket/file/transfer failures).
 *
 * Flow:
 *   1) Open log file, input file, initialize socket layer.
 *   2) Connect to server.
 *   3) While reading file chunks:
 *        - Build header (payload_size, hash)
 *        - Send header + payload
 *        - Wait for server response:
 *            * ACK   -> proceed to next chunk
 *            * RETRY -> resend same chunk (up to MAX_RETRIES)
 *   4) On EOF: send FLAG_END header.
 *   5) Cleanup and exit.
 */
int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <server_ip> <port> <input_file>\n", argv[0]);
        return 1;
    }

    const char *server_ip  = argv[1];
    int         port       = atoi(argv[2]);
    const char *input_path = argv[3];

    /* Open very simple log file */
    log_fp = fopen(LOG_FILE_PATH, "a");
    if (!log_fp) {
        printf("Failed to open log file\n");
        return 1;
    }
    logmsg("Client starting");

    /* Open input file to send */
    FILE *fp = fopen(input_path, "rb");
    if (!fp) {
        printf("Failed to open input file: %s\n", input_path);
        logmsg("Failed to open input file");
        fclose(log_fp);
        return 1;
    }

    /* Platform socket initialization (no-op on POSIX, WSAStartup on Windows) */
    socket_init();

    /* Create TCP socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("socket() failed: %s\n", strerror(errno));
        logmsg("socket() failed");
        fclose(fp);
        fclose(log_fp);
        socket_cleanup();
        return 1;
    }

    /* Server address */
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons((uint16_t)port);
    if (inet_pton(AF_INET, server_ip, &srv.sin_addr) != 1) {
        printf("Invalid server IP: %s\n", server_ip);
        logmsg("Invalid server IP");
        close(sock);
        fclose(fp);
        fclose(log_fp);
        socket_cleanup();
        return 1;
    }

    /* Connect */
    if (connect(sock, (struct sockaddr*)&srv, sizeof(srv)) < 0) {
        printf("connect() failed: %s\n", strerror(errno));
        logmsg("connect() failed");
        close(sock);
        fclose(fp);
        fclose(log_fp);
        socket_cleanup();
        return 1;
    }
    printf("Connected to %s:%d\n", server_ip, port);
    logmsg("Connected to server");

    uint8_t buffer[CHUNK_SIZE];  /* Payload buffer; size from protocol.h */
    uint8_t hashval[32];         /* SHA-256 of current chunk */
    packet_header_t header;      /* Outgoing header with payload_size/flags/hash */

    size_t chunk_index   = 0;    /* Optional: counter for chunks sent (not used in protocol here) */
    int overall_error    = 0;    /* Non-zero if an unrecoverable error occurs */

    /* Read and send the file in CHUNK_SIZE chunks */
    while (1) {
        size_t r = fread(buffer, 1, CHUNK_SIZE, fp);
        if (r == 0) {
            if (feof(fp)) {
                /* End-of-file: send END header to finish transfer */
                memset(&header, 0, sizeof(header));
                header.flags = FLAG_END;
                if (send_all(sock, &header, sizeof(header)) <= 0) {
                    printf("Failed to send END header\n");
                    logmsg("Failed to send END header");
                    overall_error = 1;
                } else {
                    printf("Transfer complete\n");
                    logmsg("Transfer complete (END sent)");
                }
                break;
            } else {
                /* File read error */
                printf("Read error on input file\n");
                logmsg("Read error on input file");
                overall_error = 1;
                break;
            }
        }

        /* Prepare header for this chunk */
        memset(&header, 0, sizeof(header));
        header.payload_size = (uint32_t)r;
        sha256(buffer, r, hashval);
        memcpy(header.hash, hashval, 32);

        int retry_count = 0;

    resend_same_chunk:
        /* Send header */
        if (send_all(sock, &header, sizeof(header)) <= 0) {
            printf("Failed to send header\n");
            logmsg("Failed to send header");
            overall_error = 1;
            break;
        }

        /* Send payload */
        if (send_all(sock, buffer, r) <= 0) {
            printf("Failed to send payload\n");
            logmsg("Failed to send payload");
            overall_error = 1;
            break;
        }

        /* Wait for server response (ACK / RETRY / END) */
        packet_header_t resp;
        ssize_t hn = recv_all(sock, &resp, sizeof(resp));
        if (hn <= 0) {
            printf("Server disconnected / recv failed\n");
            logmsg("Server disconnected or recv failed");
            overall_error = 1;
            break;
        }

        if (resp.flags & FLAG_ACK) {
            /* Server accepted chunk */
            retry_count = 0;
            chunk_index++;
            logmsg("ACK received");
            continue;

        } else if (resp.flags & FLAG_RETRY) {
            /* Server requests resend of the same chunk */
            retry_count++;
            logmsg("RETRY received");
            if (retry_count >= MAX_RETRIES) {
                printf("Max retries reached for a chunk\n");
                logmsg("Max retries reached, aborting");
                overall_error = 1;
                break;
            }
            /* Resend the same header + payload */
            goto resend_same_chunk;

        } else if (resp.flags & FLAG_END) {
            /* Uncommon: server indicates end; honor it */
            printf("Server indicated END\n");
            logmsg("Server indicated END");
            break;

        } else {
            /* Unknown/unsupported response flags */
            printf("Unknown response from server (flags=%u)\n", resp.flags);
            logmsg("Unknown response from server");
            overall_error = 1;
            break;
        }
    }

    /* Cleanup */
    close(sock);
    fclose(fp);
    fclose(log_fp);
    socket_cleanup();

    return overall_error ? 1 : 0;
}
