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
int alarmEnabled = 0;
int alarmCount = 0;
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{   
    int fd = openSerialPort(connectionParameters.serialPort,connectionParameters.baudRate);
    if(fd < 0)return -1;
    State state = START;
    alarmCount = 0;
    timeout = connectionParameters.timeout;
    unsigned char byte;
    switch(connectionParameters.role){
        case(LlTx) :{
            (void)signal(SIGALRM, alarmHandler);
            unsigned char frame[BUF_SIZE] = {FLAG,A_trans,C_SET,A_trans ^C_SET,FLAG} ;
            while (alarmCount < connectionParameters.nRetransmissions){
            if (alarmEnabled == FALSE)
            {
                write(fd, frame, BUF_SIZE);
                alarm(timeout); 
                alarmEnabled = TRUE;
            }
            while (alarmEnabled == TRUE && state != READ){
                if (read(fd,&byte,1) > 0) {
                    switch(state){
                        case START:
                            if (byte == FLAG) state = FLAG_ST; 
                            break;
                        case FLAG_ST:
                            if (byte == A_recei) state = A; 
                            break;
                        case A:
                            if(byte == C_UA) state = C;
                            break;
                        case C:
                            if(byte == (A_recei ^ C_UA)) state = BCC;
                            break;        
                        case BCC:
                            if(byte == FLAG) state = READ;
                            break;  
                        default:
                            break;         
                    }
            }   
            }
            if(state == READ)return 1;
    }
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
                            break;
                        case A:
                            if(byte == C_SET) state = C;
                            break;
                        case C:
                            if(byte == (A_trans ^ C_SET)) state = BCC;
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
                unsigned char frame[BUF_SIZE] = {FLAG,A_recei,C_UA,A_recei ^C_UA,FLAG} ;
                write(fd, frame, BUF_SIZE);
                return 1;
            }
        }
        default :
            return -1;
            break;
    }
    return -1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

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
