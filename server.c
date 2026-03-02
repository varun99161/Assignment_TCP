
#include "protocol.h"
#include "hash.h"
#include "socket_wrapper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

ssize_t recv_all(int sock, void *buf,size_t len)
{
    size_t total = 0;
    while(total < len)
    {
        ssize_t n = recv(sock, (char *)buf + total,len - total, 0);
        if(n <= 0)
        {
            return n;
        }
        total += n;
    }
    return total;
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    socket_init();
    int port = atoi(argv[1]);

    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    printf("Server listening on port %d\n", port);
    socket_t client = accept(server_fd, NULL, NULL);

    FILE *fp = fopen("received_file.bin", "wb");
    packet_header_t header;
    uint8_t buffer[CHUNK_SIZE], calc_hash[32];

    while (1) {
        recv_all(client, &header, sizeof(header));

        if(header.payload_size > CHUNK_SIZE)
        {
            printf("corrupted packet detected");
            break;
        }

        if (header.flags & FLAG_END) {
            printf("Transfer complete\n");
            break;
        }

        recv_all(client, buffer, header.payload_size);
        sha256(buffer, header.payload_size, calc_hash);

        if (memcmp(calc_hash, header.hash, 32) == 0) {
            fwrite(buffer, 1, header.payload_size, fp);
            header.flags = FLAG_ACK;
        } else {
            header.flags = FLAG_RETRY;
        }

        send(client, &header, sizeof(header), 0);
    }

    fclose(fp);
    socket_cleanup();
    return 0;
}
