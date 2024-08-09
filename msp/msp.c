#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "msp/msp.h"


static uint8_t crc8_calc(uint8_t crc, unsigned char a, uint8_t poly)
{
    crc ^= a;
    for (int ii = 0; ii < 8; ++ii) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ poly;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}


void mspSerialProcessReceivedData(mspPort_t *mspPort, uint8_t c)
{
    switch (mspPort->c_state) {
        default:
        case MSP_IDLE:      // Waiting for '$' character
            if (c == '$') {
                mspPort->c_state = MSP_HEADER_START;
            } else {
                return;
            }
            break;

        case MSP_HEADER_START:  // Waiting for 'M' (MSPv1 / MSPv2_over_v1) or 'X' (MSPv2 native)
            mspPort->offset = 0;
            mspPort->checksum1 = 0;
            mspPort->checksum2 = 0;
            switch (c) {
                case 'M':
                    mspPort->c_state = MSP_HEADER_M;
                    mspPort->msp_version = MSP_V1;
                    break;
                case 'X':
                    mspPort->c_state = MSP_HEADER_X;
                    mspPort->msp_version = MSP_V2_NATIVE;
                    break;
                default:
                    mspPort->c_state = MSP_IDLE;
                    break;
            }
            break;

        case MSP_HEADER_M:      // Waiting for '<' / '>'
            mspPort->c_state = MSP_HEADER_V1;
            switch (c) {
                case '<':
                    mspPort->packet_type = MSP_PACKET_COMMAND;
                    break;
                case '>':
                    mspPort->packet_type = MSP_PACKET_RESPONSE;
                    break;
                default:
                    mspPort->packet_type = MSP_PACKET_UNKNOWN;
                    mspPort->c_state = MSP_IDLE;
                    break;
            }
            break;

        case MSP_HEADER_X:
            mspPort->c_state = MSP_HEADER_V2_NATIVE;
            switch (c) {
                case '<':
                    mspPort->packet_type = MSP_PACKET_COMMAND;
                    break;
                case '>':
                    mspPort->packet_type = MSP_PACKET_RESPONSE;
                    break;
                default:
                    mspPort->packet_type = MSP_PACKET_UNKNOWN;
                    mspPort->c_state = MSP_IDLE;
                    break;
            }
            break;

        case MSP_HEADER_V1:     // Now receive v1 header (size/cmd), this is already checksummable
            mspPort->payload[mspPort->offset++] = c;
            mspPort->checksum1 ^= c;
            if (mspPort->offset == sizeof(mspHeaderV1_t)) {
                mspHeaderV1_t * hdr = (mspHeaderV1_t *)&mspPort->payload[0];
                // Check incoming buffer size limit
                if (hdr->size > MSP_PORT_INBUF_SIZE) {
                    mspPort->c_state = MSP_IDLE;
                }
                else if (hdr->cmd == MSP_V2_FRAME_ID) {
                    // MSPv1 payload must be big enough to hold V2 header + extra checksum
                    if (hdr->size >= sizeof(mspHeaderV2_t) + 1) {
                        mspPort->msp_version = MSP_V2_OVER_V1;
                        mspPort->c_state = MSP_HEADER_V2_OVER_V1;
                    } else {
                        mspPort->c_state = MSP_IDLE;
                    }
                } else {
                    mspPort->data_size = hdr->size;
                    mspPort->msp_cmd = hdr->cmd;
                    mspPort->cmd_flags = 0;
                    mspPort->offset = 0;                // re-use buffer
                    mspPort->c_state = mspPort->data_size > 0 ? MSP_PAYLOAD_V1 : MSP_CHECKSUM_V1;    // If no payload - jump to checksum byte
                }
            }
            break;

        case MSP_PAYLOAD_V1:
            mspPort->payload[mspPort->offset++] = c;
            mspPort->checksum1 ^= c;
            if (mspPort->offset == mspPort->data_size) {
                mspPort->c_state = MSP_CHECKSUM_V1;
            }
            break;

        case MSP_CHECKSUM_V1:
            if (mspPort->checksum1 == c) {
                mspPort->c_state = MSP_COMMAND_RECEIVED;
                if (mspPort->callback != 0) {
                    mspPort->callback(mspPort->msp_version, mspPort->msp_cmd, mspPort->data_size, mspPort->payload);
                    mspPort->c_state = MSP_IDLE;
                }
            } else {
                mspPort->c_state = MSP_IDLE;
            }
            break;

        case MSP_HEADER_V2_OVER_V1:     // V2 header is part of V1 payload - we need to calculate both checksums now
            mspPort->payload[mspPort->offset++] = c;
            mspPort->checksum1 ^= c;
            mspPort->checksum2 = crc8_calc(mspPort->checksum2, c, 0xD5);
            if (mspPort->offset == (sizeof(mspHeaderV2_t) + sizeof(mspHeaderV1_t))) {
                mspHeaderV2_t * hdrv2 = (mspHeaderV2_t *)&mspPort->payload[sizeof(mspHeaderV1_t)];
                if (hdrv2->size > MSP_PORT_INBUF_SIZE) {
                    mspPort->c_state = MSP_IDLE;
                } else {
                    mspPort->data_size = hdrv2->size;
                    mspPort->msp_cmd = hdrv2->cmd;
                    mspPort->cmd_flags = hdrv2->flags;
                    mspPort->offset = 0;                // re-use buffer
                    mspPort->c_state = mspPort->data_size > 0 ? MSP_PAYLOAD_V2_OVER_V1 : MSP_CHECKSUM_V2_OVER_V1;
                }
            }
            break;

        case MSP_PAYLOAD_V2_OVER_V1:
            mspPort->checksum2 = crc8_calc(mspPort->checksum2, c, 0xD5);
            mspPort->checksum1 ^= c;
            mspPort->payload[mspPort->offset++] = c;

            if (mspPort->offset == mspPort->data_size) {
                mspPort->c_state = MSP_CHECKSUM_V2_OVER_V1;
            }
            break;

        case MSP_CHECKSUM_V2_OVER_V1:
            mspPort->checksum1 ^= c;
            if (mspPort->checksum2 == c) {
                mspPort->c_state = MSP_CHECKSUM_V1; // Checksum 2 correct - verify v1 checksum
            } else {
                mspPort->c_state = MSP_IDLE;
            }
            break;

        case MSP_HEADER_V2_NATIVE:
            mspPort->payload[mspPort->offset++] = c;
            mspPort->checksum2 = crc8_calc(mspPort->checksum2, c, 0xD5);
            if (mspPort->offset == sizeof(mspHeaderV2_t)) {
                mspHeaderV2_t * hdrv2 = (mspHeaderV2_t *)&mspPort->payload[0];
                mspPort->data_size = hdrv2->size;
                mspPort->msp_cmd = hdrv2->cmd;
                mspPort->cmd_flags = hdrv2->flags;
                mspPort->offset = 0;                // re-use buffer
                mspPort->c_state = mspPort->data_size > 0 ? MSP_PAYLOAD_V2_NATIVE : MSP_CHECKSUM_V2_NATIVE;
            }
            break;

        case MSP_PAYLOAD_V2_NATIVE:
            mspPort->checksum2 = crc8_calc(mspPort->checksum2, c, 0xD5);
            mspPort->payload[mspPort->offset++] = c;

            if (mspPort->offset == mspPort->data_size) {
                mspPort->c_state = MSP_CHECKSUM_V2_NATIVE;
            }
            break;

        case MSP_CHECKSUM_V2_NATIVE:
            if (mspPort->checksum2 == c) {
                mspPort->c_state = MSP_COMMAND_RECEIVED;
                if (mspPort->callback != 0) {
                    mspPort->callback(mspPort->msp_version, mspPort->msp_cmd, mspPort->data_size, mspPort->payload);
                    mspPort->c_state = MSP_IDLE;
                }
            } else {
                mspPort->c_state = MSP_IDLE;
            }
            break;
    }

}

msp_error_e construct_msp_command_v1(uint8_t message_buffer[], uint8_t command, const uint8_t *payload, uint8_t size, msp_direction_e direction)
{

    uint8_t checksum;
    message_buffer[0] = '$'; // Header
    message_buffer[1] = 'M'; // MSP V1
    if (direction == MSP_OUTBOUND) {
        message_buffer[2] = '<';
    } else {
        message_buffer[2] = '>';
    }
    message_buffer[3] = size; // Payload Size
    checksum = size;
    message_buffer[4] = command; // Command
    checksum ^= command;
    for(uint8_t i = 0; i < size; i++) {
        message_buffer[5 + i] = payload[i];
        checksum ^= message_buffer[5 + i];
    }
    message_buffer[5 + size] = checksum;
    return 0;
}

uint16_t construct_msp_command_v2(uint8_t message_buffer[], uint16_t function, const uint8_t *payload, uint8_t size, mspPacketType_e msp_packet_type)
{
    uint8_t checksum = 0;
    uint8_t len = 0;

    message_buffer[0] = '$'; // Header
    message_buffer[1] = 'X'; // MSP V2
    if (msp_packet_type == MSP_PACKET_COMMAND) {
        message_buffer[2] = '<';
    } else if (msp_packet_type == MSP_PACKET_RESPONSE){
        message_buffer[2] = '>';
    } else {
        return 0;
    }

    // Pack header struct into message_buffer
    mspHeaderV2_t header;
    header.flags = 0;
    header.cmd = function;
    header.size = size;

    // Copy header buffer, adding each byte to the crc
    memcpy(message_buffer+3, &header, sizeof(mspHeaderV2_t));
    for (int i = 0; i < 5; i++) {
        checksum = crc8_calc(checksum, message_buffer[3 + i], 0xD5);
    }

    // Copy payload, adding each byte to the crc
    if (size > 0 && payload != NULL) {
        memcpy(message_buffer+8, payload, size);
        for (int i = 0; i < size; i++) {
            checksum = crc8_calc(checksum, message_buffer[8 + i], 0xD5);
        }
    }

    // Add checksum to message buffer
    message_buffer[8 + size] = checksum;
    len = 3 + 5 + size + 1;

#if 0 // debug construct packet
    printf("msp_buffer: ");
    for (int i = 0; i < len; ++i) {
        printf("0x%02X ", message_buffer[i]);
    }
    printf("crc: %02X\n", checksum);
#endif
    return len;
}
