#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "serial/serial.h"

int serial_open(const char *device, int baudrate)
{
    if (!device) {
        return -1;
    }

    int tty_fd = open(device, O_RDWR | O_NOCTTY);

    if (tty_fd == -1) {
       // perror(device);
        return -1;
    }

    // Flush away any bytes previously read or written.
    int result = tcflush(tty_fd, TCIOFLUSH);
    if (result)
    {
        perror("tcflush failed");  // just a warning, not a fatal error
    }

    // Get the current configuration of the serial port.
    struct termios options;
    result = tcgetattr(tty_fd, &options);
    if (result)
    {
        perror("tcgetattr failed");
        close(tty_fd);
        return -1;
    }

    // Turn off any options that might interfere with our ability to send and
    // receive raw binary bytes.
    options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // Set up timeouts: Calls to read() will return as soon as there is
    // at least one byte available or when 100 ms has passed.
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 0;

    // This code only supports certain standard baud rates. Supporting
    // non-standard baud rates should be possible but takes more work.
    switch (baudrate)
    {
        case 4800:   cfsetospeed(&options, B4800);   break;
        case 9600:   cfsetospeed(&options, B9600);   break;
        case 19200:  cfsetospeed(&options, B19200);  break;
        case 38400:  cfsetospeed(&options, B38400);  break;
        case 115200: cfsetospeed(&options, B115200); break;
        case 576000: cfsetospeed(&options, B576000); break;
        case 1000000: cfsetospeed(&options, B1000000); break;
        default:
            fprintf(stderr, "warning: baud rate %u is not supported, using 115200.\n", baudrate);
            cfsetospeed(&options, B115200);
            break;
    }
    cfsetispeed(&options, cfgetospeed(&options));

    result = tcsetattr(tty_fd, TCSANOW, &options);
    if (result)
    {
        perror("tcsetattr failed");
        close(tty_fd);
        return -1;
    }

    return tty_fd;
}

int serial_write(int fd, const uint8_t *data, int len)
{
    if (fd < 0 || data == NULL)
        return 0;

    return (int)write(fd, data, len);
}
