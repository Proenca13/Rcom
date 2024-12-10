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

    // Tenta analisar a resposta com sscanf
    if (sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
               &ip1, &ip2, &ip3, &ip4, &p1, &p2) != 6) {
        fprintf(stderr, "Erro: Resposta PASV mal formatada: %s\n", response);
        exit(EXIT_FAILURE);
    }

    // Montar o endereço IP no formato correto
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

    // Calcular a porta da conexão de dados
    *port = (p1 * 256) + p2;

    // Debugging: Exibir os valores interpretados
    printf("IP interpretado: %s\n", ip);
    printf("Porta interpretada: %d\n", *port);
}


int main() {
    int control_sock, data_sock;
    char buffer[BUFFER_SIZE];
    char ip[16];
    int port;
    char ftp_server[256], file_path[256], username[256], password[256];

    // Pedir ao usuário as informações
    printf("Enter the FTP server URL (e.g., mirrors.up.pt): ");
    scanf("%255s", ftp_server);

    printf("Enter the file path on the server (e.g., /debian/README.html): ");
    scanf("%255s", file_path);

    // Resolver IP usando getip.c
    if (resolve_hostname_to_ip(ftp_server, ip) != 0) {
        error("Erro ao resolver hostname");
    }
    printf("IP do servidor FTP: %s\n", ip);

    // Criar e conectar socket de controle usando clientTCP.c
    control_sock = create_socket();
    if (connect_to_server(control_sock, ip, 21) != 0) {
        error("Erro ao conectar ao servidor FTP");
    }

    // Receber mensagem de boas-vindas
    memset(buffer, 0, sizeof(buffer));
    while (1) {
        if (recv(control_sock, buffer, sizeof(buffer) - 1, 0) <= 0) {
            error("Erro ao receber mensagem do servidor");
        }

        printf("Servidor: %s", buffer);

        // Verifica se a mensagem contém o fim da sequência esperada
        if (strstr(buffer, "220 ") != NULL) {
            // Se encontramos um "220" no formato esperado, finalizamos o loop
            break;
        }
    }

    // Autenticação
    printf("Enter the username (or 'anonymous'): ");
    scanf("%255s", username);

    printf("Enter the password (or 'anonymous'): ");
    scanf("%255s", password);
    sprintf(buffer, "USER %s\r\n", username);
    write(control_sock, buffer, strlen(buffer));
    memset(buffer, 0, sizeof(buffer));
    if (recv(control_sock, buffer, sizeof(buffer) - 1, 0) > 0) {
        printf("Servidor: %s", buffer);
    }

    sprintf(buffer, "PASS %s\r\n", password);
    write(control_sock, buffer, strlen(buffer));
    memset(buffer, 0, sizeof(buffer));
    if (recv(control_sock, buffer, sizeof(buffer) - 1, 0) > 0) {
        printf("Servidor: %s", buffer);
    }
    if (strstr(buffer, "230") == NULL) {
        error("Erro na autenticação");
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

    // Criar e conectar socket de dados usando clientTCP.c
    data_sock = create_socket();
    if (connect_to_server(data_sock, ip, port) != 0) {
        error("Erro ao conectar ao socket de dados");
    }

    // Solicitar arquivo
    sprintf(buffer, "RETR %s\r\n", file_path);
    write(control_sock, buffer, strlen(buffer));
    memset(buffer, 0, sizeof(buffer));
    if (recv(control_sock, buffer, sizeof(buffer) - 1, 0) > 0) {
        printf("Servidor: %s", buffer);
    }

    // Receber arquivo
    FILE *file = fopen("downloaded_file", "w");
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
    memset(buffer, 0, sizeof(buffer));
    if (recv(control_sock, buffer, sizeof(buffer) - 1, 0) > 0) {
        printf("Servidor: %s", buffer);
    }

    close(control_sock);
    printf("Arquivo recebido e conexão encerrada.\n");
    return 0;
}
