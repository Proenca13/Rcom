// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);
                      
unsigned char * buildControlPacket(int control_field,int fileSize,unsigned char* fileName,int *controlpacketSize);
int readControlPacket(unsigned char* controlpacket,int packetSize,unsigned char* fileName);
#endif // _APPLICATION_LAYER_H_
