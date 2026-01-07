#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

/**
 * room_manager.h - Track rooms and broadcast notifications
 * CHUNG - Transport layer utility
 */

#include "protocol/protocol.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_ROOM_MEMBERS 6
#define MAX_ROOMS 100

//==============================================================================
// Data Structures
//==============================================================================

typedef struct {
    int client_fd;
    uint32_t account_id;
    bool is_ready;       // ✅ MEMORY ONLY - not in DB
} RoomMember;

typedef struct {
    int room_id;
    RoomMember members[MAX_ROOM_MEMBERS];
    int member_count;
} RoomState;

//==============================================================================
// Room Management API
//==============================================================================

/**
 * Add a client to a room
 * @param room_id - Room ID
 * @param client_fd - Client file descriptor
 * @param account_id - Account ID from DB
 * @param is_host - true if this is the host
 */
void room_add_member(int room_id, int client_fd, uint32_t account_id, bool is_host);

/**
 * Remove a client from a room (logout, kicked, etc)
 */
void room_remove_member(int room_id, int client_fd);

/**
 * Remove a client from ALL rooms (disconnect)
 */
void room_remove_member_all(int client_fd);

/**
 * Set ready state for a member
 */
void room_set_ready(int room_id, int client_fd, bool ready);

/**
 * Check if all members are ready
 * @param room_id - Room ID
 * @param out_ready_count - Output: number of ready members
 * @param out_total_count - Output: total members
 * @return true if all ready, false otherwise
 */
bool room_all_ready(int room_id, int *out_ready_count, int *out_total_count);

/**
 * Get member count in a room
 */
int room_get_member_count(int room_id);

/**
 * Broadcast a message to all members in a room
 * @param room_id - Target room
 * @param command - Notification opcode (e.g. NTF_RULES_CHANGED)
 * @param payload - Message body
 * @param payload_len - Body length
 * @param exclude_fd - Don't send to this FD (e.g. sender), or -1 to send to all
 */
void room_broadcast(int room_id, uint16_t command, const char *payload, 
                   uint32_t payload_len, int exclude_fd);

/**
 * Get all member FDs in a room
 * @param room_id - Target room
 * @param out_count - Output: number of members
 * @return Array of FDs (valid until next call)
 */
const int* room_get_members(int room_id, int *out_count);

/**
 * Get room count (for debugging)
 */
int room_get_count(void);

#endif // ROOM_MANAGER_H