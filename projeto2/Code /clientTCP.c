#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int create_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    return sock;
}

int connect_to_server(int socket, const char *ip, int port) {
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Erro ao converter IP");
        return -1;
    }

    if (connect(socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao conectar ao servidor");
        return -1;
    }

    return 0;
}
