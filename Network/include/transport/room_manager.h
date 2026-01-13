#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

/**
 * room_manager.h - Track rooms and broadcast notifications
 * CHUNG - Transport layer utility
 */

#include "protocol/protocol.h"
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#define MAX_ROOM_MEMBERS 6
#define MAX_ROOMS 100

//==============================================================================
// Enums
//==============================================================================

typedef enum {
    ROOM_WAITING = 0,
    ROOM_PLAYING = 1,
    ROOM_CLOSED = 2
} RoomStatus;

typedef enum {
    MODE_ELIMINATION = 0,
    MODE_SCORING = 1
} GameMode;

typedef enum {
    ROOM_PUBLIC = 0,
    ROOM_PRIVATE = 1
} RoomVisibility;

//==============================================================================
// Data Structures
//==============================================================================

typedef struct {
    uint32_t account_id;
    char name[64];  // Player name
    char avatar[256]; // Avatar URL
    bool is_host;
    bool is_ready;
    bool connected;
    time_t joined_at;
} RoomPlayerState;

typedef struct {
    uint32_t id;
    char name[32];
    char code[8];
    uint32_t host_id;
    
    RoomStatus status;
    GameMode mode;
    uint8_t max_players;
    RoomVisibility visibility;
    uint8_t wager_mode;
    
    // Player tracking
    RoomPlayerState players[MAX_ROOM_MEMBERS];
    uint8_t player_count;
    
    // Socket tracking (for broadcast)
    int member_fds[MAX_ROOM_MEMBERS];
    int member_count;
} RoomState;

//==============================================================================
// Room Management API
//==============================================================================

/**
 * Room lifecycle
 */
RoomState* room_create(void);
void room_destroy(uint32_t room_id);
RoomState* room_get(uint32_t room_id);

/**
 * Player management
 */
int room_add_player(uint32_t room_id, uint32_t account_id, const char *name, const char *avatar, int client_fd);
int room_remove_player(uint32_t room_id, uint32_t account_id);
bool room_has_player(uint32_t room_id, uint32_t account_id);
bool room_user_in_any_room(uint32_t account_id);

/**
 * Find room ID by player's file descriptor
 * Returns 0 if player not in any room
 */
uint32_t room_find_by_player_fd(int client_fd);

/**
 * Get room state by ID (alias for room_get for clarity)
 */
RoomState* room_get_state(uint32_t room_id);

/**
 * Find room by 6-character code
 * Returns NULL if not found
 */
RoomState* find_room_by_code(const char *code);

/**
 * Close room if empty (updates DB and destroys in-memory state)
 * Called automatically by room_remove_member when last player leaves
 */
void room_close_if_empty(uint32_t room_id);

/**
 * Add a client to a room
 */
void room_add_member(int room_id, int client_fd);

/**
 * Remove a client from a room (logout, kicked, etc)
 */
void room_remove_member(int room_id, int client_fd);

/**
 * Remove a client from ALL rooms (disconnect)
 */
void room_remove_member_all(int client_fd);

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

/**
 * Broadcast NTF_PLAYER_LIST to all members in a room
 */
void broadcast_player_list(uint32_t room_id);

#endif // ROOM_MANAGER_H
