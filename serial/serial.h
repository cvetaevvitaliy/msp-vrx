#ifndef MSP2UDP_SERIAL_H
#define MSP2UDP_SERIAL_H

#include <stdint.h>

int serial_open(const char *device, int baudrate);

int serial_write(int fd, const uint8_t *data, int len);

#endif //MSP2UDP_SERIAL_H
