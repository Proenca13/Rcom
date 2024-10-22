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
int frame_number = 0;
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
            while (alarmCount <= connectionParameters.nRetransmissions){
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
                            else state = START;
                            break;
                        case C:
                            if(byte == (A_recei ^ C_UA)) state = BCC;
                            else if (byte == FLAG) state = FLAG_ST;
                            else state = START;
                            break;        
                        case BCC:
                            if(byte == FLAG) state = READ;
                            else state = START;
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
                            else state = START;
                            break;
                        case C:
                            if(byte == (A_trans ^ C_SET)) state = BCC;
                            else if (byte == FLAG) state = FLAG_ST;
                            else state = START;
                            break;        
                        case BCC:
                            if(byte == FLAG) state = READ;
                            else state = START;
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

int llwrite(LinkLayer connectionParameters,const unsigned char *buf, int bufSize)
{
    int fd = openSerialPort(connectionParameters.serialPort,connectionParameters.baudRate);
    if(fd < 0)return -1;
    int frame_size = 6 + bufSize;
    unsigned char *frame = (unsigned char *)malloc(frame_size);
    frame[0] = FLAG;
    frame[1] = A_trans;
    if(frame_number == 0)frame[2] = I_0;
    else frame[2] = I_1;
    frame[3] = A_trans ^ frame[2];
    memcpy(frame + 4,buf, bufSize);
    unsigned char BCC2  = buf[0];
    for(int i = 1;i<= bufSize;i++){
        BCC2 = BCC2 ^ buf[i];
    }
    int j = 4;  
    for (unsigned int i = 0; i < bufSize; i++) {
        if (buf[i] == FLAG || buf[i] == ESC) {
            frame = realloc(frame, ++frame_size);
            if (frame == NULL) {
                perror("Memory allocation failed");
                return -1;  
            }
            frame[j++] = ESC;  
            frame[j++] = buf[i] ^ 0x20;  
        } 
        else {
            frame[j++] = buf[i];  
        }
    }
    if (BCC2 == FLAG || BCC2 == ESC) {
    frame = realloc(frame, ++frame_size);
    if (frame == NULL) {
        perror("Memory allocation failed");
        return -1;
    }
    frame[j++] = ESC;
    frame[j++] = BCC2 ^ 0x20; 
    } 
    else {
        frame[j++] = BCC2; 
    }
    frame[j++] = FLAG;
    alarmCount = 0;
    int accepted = 0;
    State state = START;
    unsigned char C_byte;
    unsigned char byte;
    (void)signal(SIGALRM, alarmHandler);
    while (alarmCount <= connectionParameters.nRetransmissions ){
            if (alarmEnabled == FALSE){
                write(fd, frame, frame_size);
                alarm(timeout); 
                alarmEnabled =  TRUE;
            }
            while (alarmEnabled == TRUE && state != READ){  
                if (read(fd,&byte,1) > 0){
                    switch(state){
                        case START:
                            if (byte == FLAG) state = FLAG_ST; 
                            break;
                        case FLAG_ST:
                            if (byte == A_recei) state = A; 
                            else if (byte != FLAG) state = START;
                            break;
                        case A:
                            if ( byte == C_RR0 || byte == C_RR1 || byte == C_REJ0 || byte == C_REJ1 ){
                                state = C;
                                C_byte = byte;
                            }
                            else if (byte == FLAG) state = FLAG_ST;
                            else state = START;
                            break;
                        case C:
                            if(byte == (A_recei ^ C_byte)) state = BCC;
                            else if (byte == FLAG) state = FLAG_ST;
                            else state = START;
                            break;
                        case BCC:
                            if(byte == FLAG){
                                state = READ;
                            }
                            else state = START;
                            break;
                        default:
                            break;           
                    }
                }         
            }
            if(state == READ){
                if(C_byte == C_RR0 || C_byte == C_RR1){
                    accepted = 1;
                    frame_number = (frame_number+1)%2;
                }
            }
            if(accepted){
                alarmCount = connectionParameters.nRetransmissions+1;
                alarm(0);
                alarmEnabled = FALSE;
                break;

            }
    }
    free(frame);
    if (accepted) return frame_size;
    return -1;
}


int llread(LinkLayer connectionParameters,unsigned char *packet) {
    int fd = openSerialPort(connectionParameters.serialPort,connectionParameters.baudRate);
    State state = START;
    unsigned char byte;
    unsigned char C_byte;
    int dataIndex = 0;  
    unsigned char c_response;
    while (state != READ) {
        if (read(fd, &byte, 1) <= 0) {
            continue;  
        }

        switch (state) {
            case START:
                if (byte == FLAG)
                    state = FLAG_ST;  
                break;

            case FLAG_ST:
                if (byte == A_trans)
                    state = A;  
                else if (byte != FLAG)
                    state = START;  
                break;

            case A:
                if (byte == I_0 || byte == I_1) {  
                    C_byte = byte;
                    state = C;  
                } else if (byte == FLAG)
                    state = FLAG_ST;  
                else
                    state = START;  
                break;

            case C:
                if (byte == (A_trans ^ C_byte))  
                    state = BCC;  
                else if (byte == FLAG)
                    state = FLAG_ST;  
                else
                    state = START;  
                break;

            case BCC:
                if (byte == ESC) {
                    state = ESC_FLAG_READ;  
                } 
                else if (byte == FLAG) {
                unsigned char BCC2 = packet[dataIndex - 1];  
                dataIndex--;  
                packet[dataIndex] = '\0';  
                unsigned char bcc_check = packet[0];
                for (unsigned int j = 1; j < dataIndex; j++) {
                    bcc_check ^= packet[j];
                }
                if (BCC2 == bcc_check) {
                    state = READ;  
                } 
                else {
                    if(frame_number == 0)c_response = C_REJ0;
                    else c_response = C_REJ1;
                    unsigned char frame[5] = {FLAG,A_recei,c_response,A_recei ^c_response,FLAG} ;
                    write(fd, frame, 5);
                    printf("Frame rejected.\n");
                    printf("Error: BCC2 mismatch.\n");
                    return -1;  
                }
                } 
                else {
                    packet[dataIndex++] = byte;
                }
            break;
            case ESC_FLAG_READ:
                state = BCC;
                if (byte == FLAG || byte == ESC) {
                    packet[dataIndex++] = byte;
                } else {
                    packet[dataIndex++] = byte ^ 0x20;  
                }
                break;


            default:
                break;
        }
    }
    if(frame_number == 0)c_response = C_RR0;
    else c_response = C_RR1;
    unsigned char frame[5] = {FLAG,A_recei,c_response,A_recei ^c_response,FLAG} ;
    write(fd, frame, 5);
    printf("Frame successfully received.\n");
    return dataIndex; 
}



////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(LinkLayer connectionParameters,int showStatistics)
{
    int fd = openSerialPort(connectionParameters.serialPort,connectionParameters.baudRate);
    if(fd < 0)return -1;
    State state = START;
    unsigned char byte;
    alarmCount = 0;
    int alarmEnabled = FALSE;
    
    (void)signal(SIGALRM, alarmHandler);  
    
    while (alarmCount <= connectionParameters.nRetransmissions) {
        if (alarmEnabled == FALSE) {
            unsigned char discFrame[5] = {FLAG, A_trans, C_DISC, A_trans ^ C_DISC, FLAG};
            write(fd, discFrame, 5);   
            alarm(connectionParameters.timeout);  
            alarmEnabled = TRUE;
        }

        while (alarmEnabled == TRUE && state != READ) {
            if (read(fd, &byte, 1) > 0) {
                switch (state) {
                    case START:
                        if (byte == FLAG) state = FLAG_ST;
                        break;
                    case FLAG_ST:
                        if (byte == A_recei) state = A;
                        else if (byte != FLAG) state = START;
                        break;
                    case A:
                        if (byte == C_DISC) state = C;
                        else if (byte == FLAG) state = FLAG_ST;
                        else state = START;
                        break;
                    case C:
                        if (byte == (A_recei ^ C_DISC)) state = BCC;
                        else if (byte == FLAG) state = FLAG_ST;
                        else state = START;
                        break;
                    case BCC:
                        if (byte == FLAG) state = READ;
                        else state = START;
                        break;
                    default:
                        break;
                }
            }
        }

        if (state == READ) {
            printf("Received DISC frame. Sending UA frame.\n");

            unsigned char uaFrame[5] = {FLAG, A_recei, C_UA, A_recei ^ C_UA, FLAG};
            write(fd, uaFrame, 5);
            printf("UA frame sent. Closing connection.\n");

            int closeStatus = closeSerialPort(fd);
            if (showStatistics) {
                printf("Connection closed successfully. Statistics displayed.\n");
            }

            return closeStatus;  
        }

    }

    printf("Error: Failed to receive DISC after %d retransmissions.\n", connectionParameters.nRetransmissions);
    return -1;
}
