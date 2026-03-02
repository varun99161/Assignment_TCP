
#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdint.h>

#define CHUNK_SIZE (1024 * 1024)

#define FLAG_START 0x01
#define FLAG_DATA  0x02
#define FLAG_END   0x04
#define FLAG_ACK   0x08
#define FLAG_RETRY 0x10

typedef struct {
    uint32_t seq_no;
    uint32_t payload_size;
    uint8_t  hash[32];
    uint8_t  flags;
} packet_header_t;

#endif
