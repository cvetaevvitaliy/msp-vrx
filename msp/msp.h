#ifndef MSP_MSP_H
#define MSP_MSP_H
#include <stdint.h>
#include <stdbool.h>

#define MSP_MAX_BUFFER_SIZE     256
#define MSP_PORT_INBUF_SIZE     192
#define MSP_V2_FRAME_ID         255

typedef enum {
    MSP_IDLE,
    MSP_HEADER_START,
    MSP_HEADER_M,
    MSP_HEADER_X,

    MSP_HEADER_V1,
    MSP_PAYLOAD_V1,
    MSP_CHECKSUM_V1,

    MSP_HEADER_V2_OVER_V1,
    MSP_PAYLOAD_V2_OVER_V1,
    MSP_CHECKSUM_V2_OVER_V1,

    MSP_HEADER_V2_NATIVE,
    MSP_PAYLOAD_V2_NATIVE,
    MSP_CHECKSUM_V2_NATIVE,

    MSP_COMMAND_RECEIVED
} mspState_e;

typedef enum {
    MSP_V1          = 0,
    MSP_V2_OVER_V1  = 1,
    MSP_V2_NATIVE   = 2,
    MSP_VERSION_COUNT
} mspVersion_e;

typedef enum {
    MSP_PACKET_UNKNOWN,
    MSP_PACKET_COMMAND,
    MSP_PACKET_RESPONSE
} mspPacketType_e;

typedef int mspDescriptor_t;


typedef void (*msp_msg_callback)(mspVersion_e msp_version, uint16_t msp_cmd, uint_fast16_t data_size, uint8_t *payload);

typedef struct mspPort_s {
    mspState_e c_state;
    uint8_t payload[MSP_PORT_INBUF_SIZE];
    uint16_t msp_cmd;
    uint8_t cmd_flags;
    mspVersion_e msp_version;
    uint_fast16_t offset;
    uint_fast16_t data_size;
    uint8_t checksum1;
    uint8_t checksum2;
    mspDescriptor_t descriptor;
    msp_msg_callback callback;
    mspPacketType_e packet_type;
} mspPort_t;

typedef struct __attribute__((packed)) {
    uint8_t size;
    uint8_t cmd;
} mspHeaderV1_t;

typedef struct __attribute__((packed)) {
    uint8_t  flags;
    uint16_t cmd;
    uint16_t size;
} mspHeaderV2_t;

typedef enum {
    MSP_ERR_NONE = 0,
    MSP_ERR_HDR,
    MSP_ERR_LEN,
    MSP_ERR_CKS,
    MSP_ERROR
} msp_error_e;

typedef enum {
    MSP_INBOUND,
    MSP_OUTBOUND
} msp_direction_e;

void mspSerialProcessReceivedData(mspPort_t *mspPort, uint8_t c);

msp_error_e construct_msp_command_v1(uint8_t message_buffer[], uint8_t command, const uint8_t *payload, uint8_t size, msp_direction_e direction);

uint16_t construct_msp_command_v2(uint8_t message_buffer[], uint16_t function, const uint8_t *payload, uint8_t size, mspPacketType_e msp_packet_type);


#endif //MSP_MSP_H
