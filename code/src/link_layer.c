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
    unsigned char frame[frame_size];
    frame[0] = FLAG;
    frame[1] = A_trans;
    if(frame_number == 0)frame[2] = 0x00;
    else frame[2] = 0x80;
    frame[3] = A_trans ^ C_SET;
    memcpy(frame + 4,buf, bufSize);
    unsigned char BCC2  = buf[0];
    for(int i = 1;i<= bufSize;i++){
        BCC2 = BCC2 ^ buf[i];
    }
    frame[4 + bufSize] = BCC2;
    frame[5 + bufSize] = FLAG;
    alarmCount = 0;
    int accepted = 0;
    State state = START;
    unsigned char C_byte;
    unsigned char byte;
    while (alarmCount < connectionParameters.nRetransmissions ){
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
                        case C:
                            if(byte == (A_recei ^ C_byte)) state = BCC;
                            else if (byte == FLAG) state = FLAG_ST;
                            else state = START;
                        case BCC:
                            if(byte == FLAG)state = READ;
                            else state = START;
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
            if(accepted)break;
        }
        if (accepted) return frame_size;
    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////

int llread(unsigned char *packet) {
    State state = START;
    unsigned char byte;
    unsigned char C_byte;
    int packetIndex = 0;
    unsigned char BCC2 = 0;

    // Iterate through the packet array until the frame is fully processed
    while (state != READ) {
        byte = packet[packetIndex++];  // Read the next byte from the packet

        switch (state) {
            case START:
                if (byte == FLAG)
                    state = FLAG_ST;  // Move to FLAG_ST when FLAG is received
                break;

            case FLAG_ST:
                if (byte == A_trans)
                    state = A;  // Move to A if Address (A_trans) matches
                else if (byte != FLAG)
                    state = START;  // Reset to START if byte is invalid
                break;

            case A:
                if (byte == 0x00 || byte == 0x80) {  // Expected control bytes (frame number)
                    C_byte = byte;
                    state = C;  // Move to C when control byte is valid
                } else if (byte == FLAG)
                    state = FLAG_ST;  // Restart from FLAG_ST if FLAG is received again
                else
                    state = START;  // Invalid byte, reset to START
                break;

            case C:
                if (byte == (A_trans ^ C_byte))
                    state = BCC;  // Move to BCC if XOR of A and C is correct
                else if (byte == FLAG)
                    state = FLAG_ST;  // Restart from FLAG_ST if FLAG is received
                else
                    state = START;  // Invalid BCC, reset to START
                break;

            case BCC:
                if (byte == FLAG) {
                    state = READ;  // Received final FLAG, frame is valid
                } else {
                    if (packetIndex == 1) {
                        BCC2 = byte;  // Initialize BCC2 with the first byte of the packet
                    } else {
                        BCC2 ^= byte;  // XOR each subsequent byte to calculate BCC2
                    }
                }
                break;

            default:
                state = START;  // Reset to START in case of unexpected state
                break;
        }
    }

    // Perform BCC2 check
    if (BCC2 != 0) {
        printf("Error: BCC2 mismatch.\n");
        return -1;  // Return error if BCC2 check fails
    }

    printf("Frame successfully received.\n");
    return packetIndex;  // Return the number of bytes processed from the packet
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
