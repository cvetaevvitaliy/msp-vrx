#include <stdio.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <stdint.h>
#include <unistd.h>
#include "serial/serial.h"
#include "msp/msp.h"
#include "msp/msptypes.h"
#include "elrs_backpack/elrs_backpack.h"

#define DEFAULT_SERIAL "/dev/ttyACM0"

#define SLEEP_MS 100

static volatile sig_atomic_t g_flag = 0;

static void sig_handler(int _)
{
    printf("Exit\n");
    g_flag = 1;
}

static void print_usage(char *s)
{
    printf("%s -s <serial port>\n", s);
    exit(0);
}

static void msp_rx_callback(mspVersion_e msp_version, uint16_t msp_cmd, uint_fast16_t payload_size, uint8_t *payload)
{
    printf("\n%s()\n", __FUNCTION__ );

    if (msp_version == MSP_V2_NATIVE) {

        switch (msp_cmd) {
            case MSP_ELRS_BACKPACK_GET_FIRMWARE:
                printf("Version: %s\n", payload);
                break;

            case MSP_ELRS_BACKPACK_GET_CHANNEL_INDEX:
                printf("Channel index: %d\n", payload[0]);
                break;

            case MSP_ELRS_BACKPACK_GET_FREQUENCY:
            {
                uint16_t freq = (payload[1] << 8) | payload[0];
                printf("Frequency: %d\n", freq);
            }
                break;

            case MSP_ELRS_BACKPACK_GET_RSSI:
            {
                // NOTE: range -91dbm to +5dbm, 0% = -91dbm, 100% = +5dbm
                printf("RSSI (NOTE: signal in percent):\n");
                int nun = payload[0];
                for (int i = 1; i < nun+1; i++) {
                    printf("\tSignal strength antenna[%d]: %d%%\n",i, payload[i]);
                }
            }
                break;

            case MSP_ELRS_BACKPACK_GET_BATTERY_VOLTAGE:
            {
                uint16_t vcc = (payload[1] << 8) | payload[0];
                printf("Battery voltage: %.2f\n", ((float)vcc / 1000.0f));
            }
                break;

            default:
                break;

        }
    }
}


static void test_set_frequency(int serial_fd)
{
    static uint16_t freq = 4900;
    backpack_set_frequency(serial_fd, freq);
    freq+=5;
    if (freq > 6000) {
        freq = 4900;
    }
}


int main(int argc, char *argv[])
{
    printf("Start MSP example\n");

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGKILL, sig_handler);

    char *serial_port = DEFAULT_SERIAL;
    int opt;
    struct pollfd poll_fds;

    while((opt = getopt(argc, argv, "sh")) != -1){
        switch(opt){
            case 's':
                serial_port = argv[optind];
                break;
            case 'h':
                print_usage(argv[0]);
                break;
            case '?':
                printf("unknown option: %c\n", optopt);
                print_usage(argv[0]);
                break;
        }
    }

    printf("Open serial: '%s'\n", serial_port);

    int serial_fd = serial_open(serial_port, 115200);
    if (serial_fd < 0) {
        printf("Failed to open serial port!\n");
        return 1;
    } else {
        printf("Serial port opened\n");
    }

    uint8_t serial_data[MSP_MAX_BUFFER_SIZE] = {0};
    ssize_t serial_data_size;

    mspPort_t mspPort = {0};
    mspPort.callback = msp_rx_callback;

    backpack_send_get_version(serial_fd);
    backpack_send_get_channel_index(serial_fd);

    while (!g_flag) {
        poll_fds.fd = serial_fd;
        poll_fds.events = POLLIN;
        poll(&poll_fds, 1, 500);

        // We got inbound serial data, process it as MSP data.
        if (0 < (serial_data_size = read(serial_fd, serial_data, sizeof(serial_data)))) {
            //printf("RECEIVED data! length %zd\n data: %s", serial_data_size, serial_data);
            for (ssize_t i = 0; i < serial_data_size; i++) {
                mspSerialProcessReceivedData(&mspPort, serial_data[i]);
            }

            backpack_send_get_battery_voltage(serial_fd);
            backpack_send_get_channel_index(serial_fd);
            backpack_send_get_frequency(serial_fd);
            backpack_send_get_rssi(serial_fd);

#if 0
            test_set_frequency(serial_fd);
#endif
        }
        usleep(1000 * SLEEP_MS);
    }

    close(serial_fd);

    return 0;
}
