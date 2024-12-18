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
#include <time.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int timeout = 0;
int transmitions = 0;
int alarmEnabled = FALSE;
int alarmCount = 0;
int frame_number = 0;
int errors = 0;
clock_t start_time;
int bits_recei = 0;
int bits_Sent = 0;
extern int fd;
LinkLayerRole role;
char serialPort[50];
int baudRate;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int llopen(LinkLayer connectionParameters)
{   
    baudRate = connectionParameters.baudRate;
    memcpy(serialPort, connectionParameters.serialPort, 50);
    openSerialPort(serialPort,baudRate);
    if(fd < 0)return -1;
    State state = START;
    timeout = connectionParameters.timeout;
    transmitions = connectionParameters.nRetransmissions;
    alarmCount = 0;
    unsigned char byte;
    role = connectionParameters.role;
    switch(role){
        case(LlTx) :{
            (void)signal(SIGALRM, alarmHandler);
            unsigned char frame[5] = {FLAG,A_trans,C_SET,A_trans ^C_SET,FLAG} ;
            while (alarmCount <= transmitions){
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
                return fd;
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
                return fd;
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
    int frame_size = 6 + bufSize;
    unsigned char *frame = (unsigned char *)malloc(frame_size);
    frame[0] = FLAG;
    frame[1] = A_trans;
    if(frame_number == 0)frame[2] = I_0;
    else frame[2] = I_1;
    frame[3] = A_trans ^ frame[2];
    memcpy(frame + 4,buf, bufSize);
    unsigned char BCC2  = buf[0];
    for(int i = 1;i<  bufSize;i++){
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
    alarmEnabled = FALSE;
    int accepted = 0;
    int rejected = 0;
    State state = START;
    unsigned char C_byte;
    unsigned char byte;
    (void)signal(SIGALRM, alarmHandler);
    while (alarmCount <= transmitions ){
            if (alarmEnabled == FALSE){
                if(write(fd, frame, frame_size) == -1){
                    openSerialPort(serialPort,baudRate);
                }
                bits_Sent += frame_size * 8;
                alarm(timeout); 
                alarmEnabled =  TRUE;
                rejected = 0;
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
                    bits_recei += frame_size *8;
                }
                else if(C_byte == C_REJ0 || C_byte== C_REJ1){
                    rejected = 1;
                    errors++;
                }
           
            }
            if(accepted){
                alarmCount = transmitions+1;
                alarm(0);
                alarmEnabled = FALSE;
                break;
            }
            if(rejected){
                alarm(0);
                printf("Frame rejected!\n");
                alarmHandler(SIGALRM);
                state = START;
            }
    }
    free(frame);
    if (accepted) return frame_size;
    return -1;
}


int llread(unsigned char *packet) {
    State state = START;
    unsigned char byte;
    unsigned char C_byte;
    int dataIndex = 0;  
    unsigned char c_response;
    int readByte = 0;
    while (state != READ) {
        readByte = read(fd, &byte, 1) ;
        if (readByte > 0) {
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
        else if(readByte <0) {
            openSerialPort(serialPort,baudRate);
        }
    }
    if(frame_number == 0)c_response = C_RR0;
    else c_response = C_RR1;
    unsigned char frame[5] = {FLAG,A_recei,c_response,A_recei ^c_response,FLAG} ;
    write(fd,frame,5);
    printf("Frame successfully received.\n");
    return dataIndex; 
}

int llclose(int showStatistics)
{
    State state = START;
    unsigned char byte;
    alarmCount = 0;
    alarmEnabled = FALSE;
    unsigned char discFrame[5] = {FLAG, A_trans, C_DISC, A_trans ^ C_DISC, FLAG};
    switch(role){
        case LlTx : {
            (void)signal(SIGALRM, alarmHandler);  
            while (alarmCount <= transmitions) {
                if (alarmEnabled == FALSE) {
                    write(fd, discFrame, 5);   
                    alarm(timeout);  
                    alarmEnabled = TRUE;
                }
                while (alarmEnabled == TRUE &&  state != READ){
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
                            if(byte == C_DISC) state = C;
                            else if (byte == FLAG) state = FLAG_ST;
                            else state = START;
                            break;
                        case C:
                            if(byte == (A_trans ^ C_DISC)) state = BCC;
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
                if (state == READ) {
                    printf("Received DISC frame. Sending UA frame.\n");
                    unsigned char uaFrame[5] = {FLAG, A_recei, C_UA, A_recei ^ C_UA, FLAG};
                    write(fd, uaFrame, 5);
                    printf("UA frame sent. Closing connection.\n");
                    int closeStatus = closeSerialPort();
                    if (showStatistics) {
                        double time_taken = endClock(); 
                        printf("Time elapsed: %f seconds\n", time_taken);
                        printf("Number of rejected frames: %i\n",errors);
                        double bits_Sent_second = bits_Sent/time_taken;
                        double bits_recei_second = bits_recei/time_taken;
                        printf("Bits received %i\n",bits_recei);
                        printf("Bits received per second: %f\n", bits_recei_second);
                        printf("Bits sent %i\n",bits_Sent);
                        printf("Bits sent per second %f\n", bits_Sent_second);
                        printf("Connection closed successfully. Statistics displayed.\n");
                    }

                    return closeStatus;  
                }

            }
        break;
    }
        case LlRx : {
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
                            if(byte == C_DISC) state = C;
                            else if (byte == FLAG) state = FLAG_ST;
                            else state = START;
                            break;
                        case C:
                            if(byte == (A_trans ^ C_DISC)) state = BCC;
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
            if (state == READ) {
                    printf("Received DISC frame. Sending DISC receiver frame.\n");
                    write(fd, discFrame, 5);
                    state = START;
                    while (state != READ){
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
                    if(state== READ){
                    printf("Received UA frame. Closing connection.\n");
                    int closeStatus = closeSerialPort();
                    if (showStatistics) {
                        double time_taken = endClock(); 
                        printf("Time elapsed: %f seconds\n", time_taken);
                        printf("Connection closed successfully. Statistics displayed.\n");
                    }
                    return closeStatus;
                    }  
                }
            break;
        }
        default:
            break;
    }
    printf("Error: Failed to receive DISC after %d retransmissions.\n", transmitions);
    return -1;
}

void startClock() {
    start_time = clock(); 
}

double endClock() {
    clock_t end_time = clock(); 
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    return elapsed_time;
}
