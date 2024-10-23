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
            /*
            const char *textMessage = "Se as estrelas fossem tão bonitas como tu passava as noites em claro a olhar pro ceu!";
            int messageLength = strlen(textMessage);
            
            if (llwrite(fd, (unsigned char *)textMessage, messageLength) < 0) {
                printf("Error sending the message\n");
            } else {
                printf("Message sent successfully.\n");
            }
            */
            unsigned char dataMessage[] = {
                0x01, 0x02, 0x7D, 0x03, 0x04 ,0x7e
            };
            int dataLength = sizeof(dataMessage) / sizeof(dataMessage[0]);

        
            if (llwrite(fd, dataMessage, dataLength) < 0) {
                printf("Error sending the message\n");
            } else {
                printf("Message sent successfully (data only).\n");
            }
            break;
        }
        case LlRx: {
            /*
            unsigned char buffer[BUF_SIZE];
            int bytesRead;
            bytesRead = llread(fd, buffer);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0'; 
                printf("Received message: %s\n", buffer);
            } else {
                printf("Error reading the message or no data received\n");
            }
            */
            unsigned char buffer[BUF_SIZE];
            int bytesRead;
            
            bytesRead = llread(fd, buffer);
            if (bytesRead > 0) {
                printf("Received %d bytes of data in hexadecimal:\n", bytesRead);
                for (int i = 0; i < bytesRead; i++) {
                    printf("%02X ", buffer[i]); // Print each byte in hex format
                }
                printf("\n");
            } else {
                printf("Error reading the message or no data received\n");
            }
            break;
        }
        default:
            break;
    }
    int fd1 = llclose(fd,linkLayer.role,1);
    if (fd1 < 0) {
        perror("Close error\n");
        exit(-1);
    }
    else (printf("close works\n"));
}
