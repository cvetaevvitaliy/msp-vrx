#include "elrs_backpack.h"
#include <stdint.h>
#include <stddef.h>
#include "serial/serial.h"
#include "msp/msp.h"
#include "msp/msptypes.h"

void backpack_send_get_version(int serial_fd)
{
    uint8_t msp_buffer[MSP_MAX_BUFFER_SIZE] = {0};
    uint16_t len = construct_msp_command_v2(msp_buffer, MSP_ELRS_BACKPACK_GET_FIRMWARE, NULL, 0, MSP_PACKET_COMMAND);
    serial_write(serial_fd, msp_buffer, len);
}

void backpack_send_get_channel_index(int serial_fd)
{
    uint8_t msp_buffer[MSP_MAX_BUFFER_SIZE] = {0};
    uint16_t len = construct_msp_command_v2(msp_buffer, MSP_ELRS_BACKPACK_GET_CHANNEL_INDEX, NULL, 0, MSP_PACKET_COMMAND);
    serial_write(serial_fd, msp_buffer, len);
}

void backpack_send_get_frequency(int serial_fd)
{
    uint8_t msp_buffer[MSP_MAX_BUFFER_SIZE] = {0};
    uint16_t len = construct_msp_command_v2(msp_buffer, MSP_ELRS_BACKPACK_GET_FREQUENCY, NULL, 0, MSP_PACKET_COMMAND);
    serial_write(serial_fd, msp_buffer, len);
}

void backpack_send_get_rssi(int serial_fd)
{
    uint8_t msp_buffer[MSP_MAX_BUFFER_SIZE] = {0};
    uint16_t len = construct_msp_command_v2(msp_buffer, MSP_ELRS_BACKPACK_GET_RSSI, NULL, 0, MSP_PACKET_COMMAND);
    serial_write(serial_fd, msp_buffer, len);
}

void backpack_send_get_battery_voltage(int serial_fd)
{
    uint8_t msp_buffer[MSP_MAX_BUFFER_SIZE] = {0};
    uint16_t len = construct_msp_command_v2(msp_buffer, MSP_ELRS_BACKPACK_GET_BATTERY_VOLTAGE, NULL, 0, MSP_PACKET_COMMAND);
    serial_write(serial_fd, msp_buffer, len);
}


void backpack_set_frequency(int serial_fd, uint16_t frequency)
{
    uint8_t msp_buffer[MSP_MAX_BUFFER_SIZE] = {0};
    uint8_t payload[2] = {frequency >> 0, frequency >> 8};

    uint16_t len = construct_msp_command_v2(msp_buffer, MSP_ELRS_BACKPACK_SET_FREQUENCY, payload, 2, MSP_PACKET_COMMAND);
    serial_write(serial_fd, msp_buffer, len);
}
