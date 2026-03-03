#ifndef PROTOCOL_H
#define PROTOCOL_H

/**
 * @file protocol.h
 * @brief Protocol definitions for chunked file transfer with integrity and flow control.
 *
 * Defines:
 *  - CHUNK_SIZE: maximum payload size per packet.
 *  - Bitwise flags used to signal control information.
 *  - packet_header_t: header placed in front of every payload.
 *
 * Typical flow:
 *   1) Client sends DATA packets (header + payload) and waits for ACK/RETRY.
 *   2) Server validates each chunk (SHA-256 over payload vs. header.hash).
 *   3) On success, server writes payload and replies with FLAG_ACK.
 *   4) On mismatch/invalid size, server replies with FLAG_RETRY.
 *   5) Transfer ends when client sends a header with FLAG_END (no payload).
 */

#include <stdint.h>

/**
 * @def CHUNK_SIZE
 * @brief Maximum size (in bytes) of the payload for each packet.
 *
 * Adjust to tune throughput vs. memory usage. Both client and server must
 * compile with the same value.
 */
#define CHUNK_SIZE (1024 * 1024)  /* 1 MiB */

/* ------------------------------------------------------------------------- */
/*                                  FLAGS                                    */
/* ------------------------------------------------------------------------- */

/**
 * @brief Control flags (bitfield) carried in packet_header_t::flags.
 *
 * Notes:
 *  - Flags may be combined (bitwise OR) if your flow requires it.
 *  - In the current simple protocol, typically one flag is used per header.
 */
#define FLAG_START 0x01  /**< (Optional) Marks the beginning of a transfer/session. */
#define FLAG_DATA  0x02  /**< (Optional) Marks a data-bearing packet. */
#define FLAG_END   0x04  /**< Signals end-of-transfer (header only, zero payload). */
#define FLAG_ACK   0x08  /**< Acknowledges successful receipt/validation of a chunk. */
#define FLAG_RETRY 0x10  /**< Requests retransmission of the last chunk. */

/* ------------------------------------------------------------------------- */
/*                             PACKET HEADER                                 */
/* ------------------------------------------------------------------------- */

/**
 * @struct packet_header_t
 * @brief Minimal header sent before each payload block.
 *
 * Fields:
 *  - seq_no:       Optional monotonic sequence number (0-based). Helps detect
 *                  duplicates/mis-ordered packets when you extend the protocol.
 *  - payload_size: Number of valid payload bytes that follow this header.
 *                  Must be 0 when FLAG_END is set. Must be <= CHUNK_SIZE.
 *  - hash[32]:     SHA-256 digest of the payload bytes for integrity checking.
 *                  For FLAG_END, this may be zeroed by the sender.
 *  - flags:        Bitfield of FLAG_* values (ACK, RETRY, END, etc.).
 *
 * Endianness:
 *  - All integer fields are currently used as host-endian within the same host
 *    architecture. If you extend this protocol over heterogeneous systems,
 *    consider sending these in network byte order (big-endian).
 */
typedef struct {
    uint32_t seq_no;        /**< Sequence number for the chunk (optional/unused by simple flow). */
    uint32_t payload_size;  /**< Size of following payload in bytes (<= CHUNK_SIZE). */
    uint8_t  hash[32];      /**< SHA-256 of the payload. */
    uint8_t  flags;         /**< Control flags: FLAG_ACK/RETRY/END/etc. */
} packet_header_t;

#endif /* PROTOCOL_H */
