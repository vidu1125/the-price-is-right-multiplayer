#ifndef PACKET_FORMAT_H
#define PACKET_FORMAT_H

#include <stdint.h>

#define PROTOCOL_VERSION 1
#define MAX_PACKET_SIZE 4096
#define MAGIC_NUMBER 0x54504952

typedef enum {
    PKT_CONNECT = 0x01,
    PKT_AUTH = 0x10,
    PKT_CREATE_ROOM = 0x20,
    PKT_JOIN_ROOM = 0x21,
    PKT_START_GAME = 0x23,
    PKT_PLAYER_BID = 0x31,
    PKT_ERROR = 0xFF
} PacketType;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t version;
    uint8_t type;
    uint16_t flags;
    uint32_t sequence;
    uint32_t payload_length;
} PacketHeader;

typedef struct {
    PacketHeader header;
    uint8_t payload[MAX_PACKET_SIZE];
} Packet;

#endif
