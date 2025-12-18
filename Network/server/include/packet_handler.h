// Network/server/include/packet_handler.h
#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <stddef.h>      // For size_t
#include <stdbool.h>     // For bool
#include "../../protocol/packet_format.h"
#include "../../protocol/commands.h"
#include "../../protocol/payloads.h"

// Send functions
bool send_response(int client_fd, uint16_t response_code, void* payload, uint32_t payload_size);
bool send_error(int client_fd, uint16_t error_code, const char* message);
bool send_notification(int client_fd, uint16_t notification_code, void* payload, uint32_t payload_size);

// Receive functions
bool recv_full(int fd, void* buffer, size_t length);
bool recv_packet_header(int client_fd, PacketHeader* header);
bool recv_packet_payload(int client_fd, uint32_t length, void** payload_out);

// Helper functions
uint16_t packet_get_command(PacketHeader* header);
uint32_t packet_get_length(PacketHeader* header);

#endif // PACKET_HANDLER_H
