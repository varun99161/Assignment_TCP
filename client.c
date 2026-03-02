
#include "protocol.h"
#include "hash.h"
#include "socket_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <server_ip> <port> <file>\n", argv[0]);
        return 1;
    }

    socket_init();
    FILE *fp = fopen(argv[3], "rb");
    if (!fp) {
        perror("File open failed");
        return 1;
    }

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    packet_header_t header = {0};
    uint8_t buffer[CHUNK_SIZE];
    size_t bytes;

    while ((bytes = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        sha256(buffer, bytes, header.hash);
        header.seq_no++;
        header.payload_size = bytes;
        header.flags = FLAG_DATA;

        send(sock, &header, sizeof(header), 0);
        send(sock, buffer, bytes, 0);
        recv(sock, &header, sizeof(header), 0);

        if (header.flags & FLAG_RETRY) {
            fseek(fp, -bytes, SEEK_CUR);
        }
    }

    header.flags = FLAG_END;
    send(sock, &header, sizeof(header), 0);

    fclose(fp);
    socket_cleanup();
    return 0;
}
