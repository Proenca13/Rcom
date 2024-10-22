// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer linkLayer;
    strcpy(linkLayer.serialPort,serialPort);
    linkLayer.role = strcmp(role, "tx") ? LlRx : LlTx;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    int fd = llopen(linkLayer);
    if (fd < 0) {
        perror("Connection error\n");
        exit(-1);
    }
    else (printf("llopen works\n"));
    switch(linkLayer.role){
        case LlTx: {
            printf("App layer transmitter works.\n");

            // Define a simple text message to send
            const char *textMessage = "Se as estrelas fossem tão bonitas como tu passava as noites em claro a olhar pro ceu!";
            int messageLength = strlen(textMessage);

            // Send the text message via llwrite
            if (llwrite(linkLayer, (unsigned char *)textMessage, messageLength) < 0) {
                printf("Error sending the message\n");
            } else {
                printf("Message sent successfully.\n");
            }
            break;
        }
        case LlRx: {
            printf("App layer receiver works.\n");

            unsigned char buffer[BUF_SIZE];
            int bytesRead;

            // Receive the message via llread
            bytesRead = llread(linkLayer, buffer);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0'; // Null-terminate the received text
                printf("Received message: %s\n", buffer);
            } else {
                printf("Error reading the message or no data received\n");
            }
            break;
        }
        default:
            break;
    }
    int fd1 = llclose(linkLayer,0);
    if (fd1 < 0) {
        perror("Close error\n");
        exit(-1);
    }
    else (printf("close works\n"));
}
