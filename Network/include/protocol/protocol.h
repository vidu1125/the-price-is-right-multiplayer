#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

//==============================================================================
// PROTOCOL CONSTANTS
//==============================================================================
#define MAGIC_NUMBER 0x4347
#define PROTOCOL_VERSION 0x01
#define HEADER_SIZE 16
#define BUFF_SIZE 4096
#define MAX_PAYLOAD_SIZE 4096



#if defined(__GNUC__) || defined(__clang__)
#define PACKED __attribute__((packed))
#else
#define PACKED
#pragma pack(push, 1)
#endif

typedef struct PACKED {
    uint16_t magic;
    uint8_t version;
    uint8_t flags;
    uint16_t command;
    uint16_t reserved;
    uint32_t seq_num;
    uint32_t length;
} MessageHeader;

void forward_response(
    int client_fd,
    MessageHeader *req,
    uint16_t cmd,
    const char *payload,
    uint32_t payload_len
);


#if !defined(__GNUC__) && !defined(__clang__)
#pragma pack(pop)
#endif

#endif // PROTOCOL_H
