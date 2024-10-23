#include "application_layer.h"
#include "link_layer.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <sys/stat.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer linkLayer;
    strcpy(linkLayer.serialPort, serialPort);
    linkLayer.role = strcmp(role, "tx") ? LlRx : LlTx;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    int fd = llopen(linkLayer);
    if (fd < 0) {
        perror("Connection error\n");
        exit(-1);
    } else {
        printf("llopen works\n");
    }

    switch (linkLayer.role) {
        case LlTx: {
            FILE *file = fopen(filename, "rb");
            if (file == NULL) {
                perror("Error opening file\n");
                exit(-1);
            }
            struct stat st;
            if (stat(filename, &st) != 0) {
                perror("Error calculating file size\n");
                exit(-1);
            }
            int fileSize = st.st_size;
            printf("Number of bytes: %d\n",fileSize);
            int startPacketSize;
            unsigned char *startControlPacket = buildControlPacket(1, fileSize, filename, &startPacketSize);
            if (startControlPacket == NULL) {
                perror("Error building start control packet\n");
                exit(-1);
            }
            if (llwrite(fd, startControlPacket, startPacketSize) < 0) {
                printf("Error sending control packet (START)\n");
                free(startControlPacket); 
                exit(-1);
            } else {
                printf("Control packet (START) sent successfully.\n");
            }
            free(startControlPacket);

            // Send file content (not implemented here)

            fclose(file); 

            int endPacketSize;
            unsigned char *endControlPacket = buildControlPacket(3, fileSize, filename, &endPacketSize);
            if (endControlPacket == NULL) {
                perror("Error building end control packet\n");
                exit(-1);
            }
            if (llwrite(fd, endControlPacket, endPacketSize) < 0) {
                printf("Error sending control packet (END)\n");
                free(endControlPacket); 
                exit(-1);
            } else {
                printf("Control packet (END) sent successfully.\n");
            }
            free(endControlPacket); 
            break;
        }
        case LlRx: {
            unsigned char buffer[MAX_PAYLOAD_SIZE];
            int bytesRead = -1;
            int endOfTransmission = 0;
            while((bytesRead = llread(fd, buffer))<0);
            int fileSize = 0;
            unsigned char* fileName = readControlPacket(buffer, bytesRead, &fileSize); 
            //FILE *write_file = fopen(fileName, "wb");
            while (!endOfTransmission) {
                bytesRead = llread(fd, buffer);
                if (bytesRead < 0) {
                    printf("Error receiving data\n");
                    exit(-1);
                }

                if (buffer[0] == 1) { 
                    printf("Received START control packet\n");
                } else if (buffer[0] == 3) { 
                    printf("Received END control packet\n");
                    printf("Filename: %s, Size: %d bytes\n", fileName, fileSize); 
                    endOfTransmission = 1;
                } else {
                    printf("Received data packet\n");
                }
            }

            break;
        }
        default:
            break;
    }
    int result = llclose(fd, linkLayer.role, 1);
    if (result < 0) {
        perror("Close error\n");
        exit(-1);
    } else {
        printf("Connection closed successfully\n");
    }
}

unsigned char* buildControlPacket(int control_field, int fileSize, const char* fileName, int *controlpacketSize) {
    int fileNameLength = strlen(fileName);
    int packetSize = 1 + 2 + sizeof(int) + 2 + fileNameLength;

    *controlpacketSize = packetSize;

    unsigned char *controlpacket = (unsigned char*) malloc(packetSize);
    if (controlpacket == NULL) {
        printf("Memory allocation failed!\n");
        return NULL;
    }

    int offset = 0;

    controlpacket[offset++] = control_field;

    controlpacket[offset++] = 0x00;              
    controlpacket[offset++] = sizeof(int);       
    memcpy(&controlpacket[offset], &fileSize, sizeof(int)); 
    offset += sizeof(int);

    controlpacket[offset++] = 0x01;             
    controlpacket[offset++] = fileNameLength;   
    memcpy(&controlpacket[offset], fileName, fileNameLength); 

    return controlpacket;
}

unsigned char * readControlPacket(unsigned char* controlpacket,int packetSize,int* fileSize){
    unsigned char number_bytes_file = controlpacket[2];
    unsigned char bytes_size[number_bytes_file]; 
    memcpy(bytes_size,controlpacket+3,number_bytes_file);
    int result = 0; 
    for (int i = 0; i < number_bytes_file; ++i) {
        result |= bytes_size[i] << (8 * i); 
    }
    *fileSize = result;
    printf("Number of bytes: %d\n", result);
    unsigned char* m = &controlpacket[3+number_bytes_file+2];
    return m;
}
