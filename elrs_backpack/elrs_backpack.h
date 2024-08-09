#ifndef MSP_EXAMPLE_ELRS_BACKPACK_H
#define MSP_EXAMPLE_ELRS_BACKPACK_H

#include <stdint.h>

void backpack_send_get_version(int serial_fd);
void backpack_send_get_channel_index(int serial_fd);
void backpack_send_get_frequency(int serial_fd);
void backpack_send_get_rssi(int serial_fd);
void backpack_send_get_battery_voltage(int serial_fd);

void backpack_set_frequency(int serial_fd, uint16_t frequency);

#endif //MSP_EXAMPLE_ELRS_BACKPACK_H
