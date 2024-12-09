#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "getip.c"       // Funções do ficheiro getip.c
#include "clientTCP.c"   // Funções do ficheiro clientTCP.c

#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
void parse_pasv_response(const char *response, char *ip, int *port) {
    int ip1, ip2, ip3, ip4, p1, p2;

    // Extraindo os números da resposta PASV
    sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip1, &ip2, &ip3, &ip4, &p1, &p2);

    // Construir o IP
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

    // Construir o número da porta
    *port = p1 * 256 + p2;
}
int main() {
    int control_sock, data_sock;
    char buffer[BUFFER_SIZE];
    char ip[16];
    int port;

    // Configurações do servidor FTP
    const char *ftp_server = "mirrors.up.pt";
    int ftp_port = 21;

    // Resolver IP usando getip.c
    if (resolve_hostname_to_ip(ftp_server, ip) != 0) {
        error("Erro ao resolver hostname");
    }
    printf("IP do servidor FTP: %s\n", ip);

    // Criar e conectar socket de controle usando clientTCP.c
    control_sock = create_socket();
    if (connect_to_server(control_sock, ip, ftp_port) != 0) {
        error("Erro ao conectar ao servidor FTP");
    }

    // Receber mensagem de boas-vindas
    memset(buffer, 0, sizeof(buffer)); 
    if (recv(control_sock, buffer, sizeof(buffer) - 1, 0) > 0) {
        printf("Servidor: %s", buffer);
    }

    // Autenticação
    write(control_sock, "USER anonymous\r\n", 16);
    memset(buffer, 0, sizeof(buffer));
    if (recv(control_sock, buffer, sizeof(buffer) - 1, 0) > 0) {
        printf("Servidor: %s", buffer);
    }

    write(control_sock, "PASS anonymous\r\n", 17);
    memset(buffer, 0, sizeof(buffer));
    if (recv(control_sock, buffer, sizeof(buffer) - 1, 0) > 0) {
        printf("Servidor: %s", buffer);
    }

// Configurar modo passivo
write(control_sock, "PASV\r\n", 6);
memset(buffer, 0, sizeof(buffer));
if (recv(control_sock, buffer, sizeof(buffer) - 1, 0) > 0) {
    printf("Servidor: %s", buffer);
}

// Interpretar resposta PASV
parse_pasv_response(buffer, ip, &port);
printf("Conexão de dados em %s:%d\n", ip, port);


    // Configurar modo passivo
    write(control_sock, "PASV\r\n", 6);
    read(control_sock, buffer, BUFFER_SIZE);
    printf("Servidor: %s", buffer);

    // Interpretar resposta PASV
    parse_pasv_response(buffer, ip, &port);
    printf("Conexão de dados em %s:%d\n", ip, port);

    // Criar e conectar socket de dados usando clientTCP.c
    data_sock = create_socket();
    if (connect_to_server(data_sock, ip, port) != 0) {
        error("Erro ao conectar ao socket de dados");
    }

    // Solicitar arquivo
    write(control_sock, "RETR debian/README.html\r\n", 25);
    read(control_sock, buffer, BUFFER_SIZE);
    printf("Servidor: %s", buffer);

    // Receber arquivo
    FILE *file = fopen("README.html", "w");
    if (!file)
        error("Erro ao criar arquivo local");

    int bytes_read;
    while ((bytes_read = read(data_sock, buffer, BUFFER_SIZE)) > 0) {
        fwrite(buffer, 1, bytes_read, file);
    }
    fclose(file);
    close(data_sock);

    // Finalizar conexão de controle
    write(control_sock, "QUIT\r\n", 6);
    read(control_sock, buffer, BUFFER_SIZE);
    printf("Servidor: %s", buffer);

    close(control_sock);
    printf("Arquivo recebido e conexão encerrada.\n");
    return 0;
}

