// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int timeout = 0;
int alarmEnabled = 0;
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
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        return -1;
    }
    State state = START;
    int alarmCount = 0;
    unsigned char frame[BUF_SIZE] = {FLAG,A_trans,C_SET,A_trans ^C_SET,FLAG} ;
    timeout = connectionParameters.timeout;
    unsigned char byte;
    switch(connectionParameters.role){
        case(LlTx) :{
            (void)signal(SIGALRM, alarmHandler);
            while (alarmCount < connectionParameters.nRetransmissions){
            if (alarmEnabled == FALSE)
            {
                int bytes = write(fd, buf, BUF_SIZE);
                alarm(timeout); 
                alarmEnabled = TRUE;
            }
            while (alarmEnabled == TRUE && state != READ){
                if (read(fd,&byte,1) > 0) {
                    switch(state){
                        case START:
                            if (byte == FLAG) state = FLAG; 
                            break;
                        case FLAG:
                            if (byte == A_recei) state = A; 
                            break;
                        case A:
                            if(byte == C_UA) state = C;
                            break;
                        case C:
                            if(byte == (A_recei ^ C_UA)) state = BCC;
                            break;        
                        case BCC:
                            if{
                                (byte == FLAG) state = READ;
                            }
                            break;   
                    }
            }   
            }
            if(state == READ)return 1;
    }
    }
        case (LlRx) :{

        }
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
