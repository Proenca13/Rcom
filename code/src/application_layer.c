// Application layer protocol implementation

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
            FILE *file = fopen(filename, "rb"); 
            if (file == NULL) {
                perror("Erro ao abrir o ficheiro\n");
                exit(-1);
            }
            struct stat st;
            if (fstat(fd, &st) != 0) {
                perror("Erro ao calcular o tamanho do ficheiro\n");
                exit(-1);
            }
            int fileSize = st.st_size;
            unsigned char startControlPacket[];
            int startPacketSize = buildControlPacket(1, fileSize, filename,startControlPacket);  
            if (llwrite(fd, startControlPacket, startPacketSize) < 0) {
                printf("Erro ao enviar o pacote de controle (START)\n");
                exit(-1);
            } else {
                printf("Pacote de controle (START) enviado com sucesso.\n");
            }
            
            // Enviar o conteúdo do ficheiro
            fclose(file);

            unsigned char endControlPacket[] ;
            int endPacketSize =  buildControlPacket(3,fileSize, filename, endControlPacket);  
            if (llwrite(fd, endControlPacket, endPacketSize) < 0) {
                printf("Erro ao enviar o pacote de controle (END)\n");
                exit(-1);
            } else {
                printf("Pacote de controle (END) enviado com sucesso.\n");
            }

            break;
        }
        case LlRx: {
            unsigned char buffer[MAX_PAYLOAD_SIZE];  
            int bytesRead = -1;
            while((bytesRead = llread(fd, buffer))<0); 
            int endOfTransmission = 0;
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
unsigned char * buildControlPacket(int control_field,int fileSize,unsigned char* fileName,int *controlpacketSize){
    int offset = 0;

    controlpacket[offset++] = control_field;

    controlpacket[offset++] = 0x00;              
    controlpacket[offset++] = sizeof(int);       
    memcpy(&controlpacket[offset], &fileSize, sizeof(int)); 
    offset += sizeof(int);                        

    int fileNameLength = strlen((char*)fileName); 
    controlpacket[offset++] = 0x01;             
    controlpacket[offset++] = fileNameLength;    
    memcpy(&controlpacket[offset], fileName, fileNameLength); 
    offset += fileNameLength;                    

    return offset;
}
int readControlPacket(unsigned char* controlpacket,int packetSize,unsigned char* fileName){
    return -1;
}

