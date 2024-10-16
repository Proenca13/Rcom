// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int timeout = 0;
int alarmEnabled = FALSE;
int alarmCount = 0;
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}
int llopen(LinkLayer connectionParameters)
{   
    int fd = openSerialPort(connectionParameters.serialPort,connectionParameters.baudRate);
    if(fd < 0)return -1;
    State state = START;
    timeout = connectionParameters.timeout;
    alarmCount = 0;
    unsigned char byte;
    switch(connectionParameters.role){
        case(LlTx) :{
            (void)signal(SIGALRM, alarmHandler);
            unsigned char frame[5] = {FLAG,A_trans,C_SET,A_trans ^C_SET,FLAG} ;
            while (alarmCount < connectionParameters.nRetransmissions ){
            if (alarmEnabled == FALSE){
                write(fd, frame, 5);
                alarm(timeout); 
                alarmEnabled =  TRUE;
            }
            while (alarmEnabled == TRUE && state != READ){
                if (read(fd,&byte,1) > 0) {
                    switch(state){
                        case START:
                            if (byte == FLAG) state = FLAG_ST; 
                            break;
                        case FLAG_ST:
                            if (byte == A_recei) state = A; 
                            else if (byte != FLAG) state = START;
                            break;
                        case A:
                            if(byte == C_UA) state = C;
                            else if (byte == FLAG) state = FLAG_ST;
                            break;
                        case C:
                            if(byte == (A_recei ^ C_UA)) state = BCC;
                            else if (byte == FLAG) state = FLAG_ST;
                            break;        
                        case BCC:
                            if(byte == FLAG) state = READ;
                            break;  
                        default:
                            break;         
                    }
            }   
            }
            if(state == READ){
                printf("Received UA frame. Connection established.\n");
                return 1;
            }
        }
        break;
    }
        case (LlRx) :{
            while (state != READ){
                if (read(fd,&byte,1) > 0) {
                    switch(state){
                        case START:
                            if (byte == FLAG) state = FLAG_ST; 
                            break;
                        case FLAG_ST:
                            if (byte == A_trans) state = A; 
                            else if (byte != FLAG) state = START;
                            break;
                        case A:
                            if(byte == C_SET) state = C;
                            else if (byte == FLAG) state = FLAG_ST;
                            break;
                        case C:
                            if(byte == (A_trans ^ C_SET)) state = BCC;
                            else if (byte == FLAG) state = FLAG_ST;
                            break;        
                        case BCC:
                            if(byte == FLAG) state = READ;
                            break;  
                        default:
                            break;         
                    }
            }   
            }
            if(state ==READ){
                unsigned char frame[5] = {FLAG,A_recei,C_UA,A_recei ^C_UA,FLAG} ;
                write(fd, frame, 5);
                printf("Sent UA frame. Connection established.\n");
                return 1;
            }
            break;
        }
        default :
            return -1;
            break;
    }
    return -1;
}

int llwrite(const unsigned char *buf, int bufSize)
{
    int fd = openSerialPort(connectionParameters.serialPort,connectionParameters.baudRate);
    if(fd < 0)return -1;
    int frame_size = 6+bufSize;
    unsigned char frame[frame_size];
    frame[0] = FLAG;
    frame[1] = A_trans;
    frame[2] = C_SET;
    frame[3] = A_trans ^C_SET;
    memcpy(frame+4,buf, bufSize);
    unsigned char BCC2  = buf[0];
    for(int i = 1;i<= bufSizeize;i++){
        BCC2 = BCC2 ^ buf[i];
    }
    frame[4+bufSize] = BCC2;
    frame[5+bufSize] = FLAG;
    alarmCount = 0;
    while (alarmCount < connectionParameters.nRetransmissions ){
            if (alarmEnabled == FALSE){
                write(fd, frame, frame_size);
                alarm(timeout); 
                alarmEnabled =  TRUE;
            }
            while (alarmEnabled == TRUE){           
            }
        }
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}
