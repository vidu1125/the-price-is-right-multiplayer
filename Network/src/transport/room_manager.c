#include "transport/room_manager.h"
#include "protocol/protocol.h"
#include "protocol/opcode.h"
#include "db/core/db_client.h"
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>

//==============================================================================
// Global State
//==============================================================================

static RoomState g_rooms[MAX_ROOMS];
static int g_room_count = 0;

//==============================================================================
// Internal Helpers
//==============================================================================

static RoomState* find_room(uint32_t room_id) {
    for (int i = 0; i < g_room_count; i++) {
        if (g_rooms[i].id == room_id) {
            return &g_rooms[i];
        }
    }
    return NULL;
}

//==============================================================================
// Room Lifecycle
//==============================================================================

RoomState* room_create(void) {
    if (g_room_count >= MAX_ROOMS) {
        return NULL;
    }
    
    RoomState *room = &g_rooms[g_room_count++];
    memset(room, 0, sizeof(RoomState));
    
    return room;
}

void room_destroy(uint32_t room_id) {
    for (int i = 0; i < g_room_count; i++) {
        if (g_rooms[i].id == room_id) {
            // Shift remaining rooms
            for (int j = i; j < g_room_count - 1; j++) {
                g_rooms[j] = g_rooms[j + 1];
            }
            g_room_count--;
            return;
        }
    }
}

RoomState* room_get(uint32_t room_id) {
    return find_room(room_id);
}

//==============================================================================
// Player Management
//==============================================================================

int room_add_player(uint32_t room_id, uint32_t account_id, const char *name, int client_fd) {
    RoomState *room = find_room(room_id);
    if (!room) return -1;
    
    if (room->player_count >= MAX_ROOM_MEMBERS) return -1;
    
    RoomPlayerState *player = &room->players[room->player_count];
    player->account_id = account_id;
    strncpy(player->name, name ? name : "Player", sizeof(player->name) - 1);
    player->name[sizeof(player->name) - 1] = '\0';
    player->is_host = false;
    player->is_ready = false;
    player->connected = true;
    player->joined_at = time(NULL);
    
    room->member_fds[room->player_count] = client_fd;
    room->player_count++;
    room->member_count++;
    
    printf("[ROOM] Added player: id=%u, name='%s' to room %u\n", account_id, player->name, room_id);
    
    return 0;
}

int room_remove_player(uint32_t room_id, uint32_t account_id) {
    RoomState *room = find_room(room_id);
    if (!room) return -1;
    
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].account_id == account_id) {
            // Shift remaining players
            for (int j = i; j < room->player_count - 1; j++) {
                room->players[j] = room->players[j + 1];
                room->member_fds[j] = room->member_fds[j + 1];
            }
            room->player_count--;
            room->member_count--;
            return 0;
        }
    }
    return -1;
}

bool room_has_player(uint32_t room_id, uint32_t account_id) {
    RoomState *room = find_room(room_id);
    if (!room) return false;
    
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].account_id == account_id) {
            return true;
        }
    }
    return false;
}

bool room_user_in_any_room(uint32_t account_id) {
    for (int i = 0; i < g_room_count; i++) {
        if (room_has_player(g_rooms[i].id, account_id)) {
            return true;
        }
    }
    return false;
}

uint32_t room_find_by_player_fd(int client_fd) {
    for (int i = 0; i < g_room_count; i++) {
        for (int j = 0; j < g_rooms[i].member_count; j++) {
            if (g_rooms[i].member_fds[j] == client_fd) {
                return g_rooms[i].id;
            }
        }
    }
    return 0;
}

RoomState* room_get_state(uint32_t room_id) {
    return find_room(room_id);
}

//==============================================================================
// Room Cleanup Helper
//==============================================================================

void room_close_if_empty(uint32_t room_id) {
    printf("[ROOM_CLOSE] Checking room %u...\n", room_id);
    
    RoomState *room = find_room(room_id);
    if (!room) {
        printf("[ROOM_CLOSE] ✗ Room %u not found in memory\n", room_id);
        return;
    }
    
    printf("[ROOM_CLOSE] Room state:\n");
    printf("  - room_id: %u\n", room->id);
    printf("  - room_name: %s\n", room->name);
    printf("  - player_count: %d\n", room->player_count);
    printf("  - member_count: %d\n", room->member_count);
    
    if (room->member_count > 0) {
        printf("[ROOM_CLOSE] Room %u still has %d members, NOT closing\n", 
               room_id, room->member_count);
        return;
    }
    
    printf("[ROOM_CLOSE] ⚠️  Room %u is EMPTY, closing...\n", room_id);
    
    // 1. Close in database
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "status", "closed");
    
    char filter[64];
    snprintf(filter, sizeof(filter), "id=eq.%u", room_id);
    
    cJSON *response = NULL;
    db_error_t rc = db_patch("rooms", filter, payload, &response);
    
    if (rc == DB_OK) {
        printf("[ROOM_CLOSE] ✓ Room %u status='closed' in DB\n", room_id);
    } else {
        printf("[ROOM_CLOSE] ✗ Failed to close room %u in DB: rc=%d\n", room_id, rc);
    }
    
    cJSON_Delete(payload);
    if (response) cJSON_Delete(response);
    
    // 2. Destroy in-memory state
    printf("[ROOM_CLOSE] Destroying in-memory state...\n");
    room_destroy(room_id);
    printf("[ROOM_CLOSE] ✓ Room %u destroyed from memory\n", room_id);
}

//==============================================================================
// Legacy API (kept for compatibility)
//==============================================================================

void room_add_member(int room_id, int client_fd) {
    RoomState *room = find_room((uint32_t)room_id);
    
    // Create room if not exists (legacy behavior)
    if (!room) {
        room = room_create();
        if (!room) {
            printf("[ROOM] Cannot create room %d - max limit reached\\n", room_id);
            return;
        }
        room->id = (uint32_t)room_id;
    }
    
    // Check if already in room
    for (int i = 0; i < room->member_count; i++) {
        if (room->member_fds[i] == client_fd) {
            return; // Already member
        }
    }
    
    // Add member
    if (room->member_count < MAX_ROOM_MEMBERS) {
        room->member_fds[room->member_count++] = client_fd;
        printf("[ROOM] Added fd=%d to room=%d (%d members)\\n", 
               client_fd, room_id, room->member_count);
    } else {
        printf("[ROOM] Room %d is full\\n", room_id);
    }
}

void room_remove_member(int room_id, int client_fd) {
    RoomState *room = find_room((uint32_t)room_id);
    if (!room) return;
    
    // Find and remove
    for (int i = 0; i < room->member_count; i++) {
        if (room->member_fds[i] == client_fd) {
            // Shift remaining members
            for (int j = i; j < room->member_count - 1; j++) {
                room->member_fds[j] = room->member_fds[j + 1];
            }
            room->member_count--;
            printf("[ROOM] Removed fd=%d from room=%d (%d members left)\n",
                   client_fd, room_id, room->member_count);
            
            // Close room if empty (with DB sync)
            if (room->member_count == 0) {
                printf("[ROOM] Member count reached 0, calling room_close_if_empty()\n");
                room_close_if_empty((uint32_t)room_id);
            }
            
            return;
        }
    }
}

void room_remove_member_all(int client_fd) {
    // Remove from all rooms (when client disconnects)
    for (int i = 0; i < g_room_count; i++) {
        RoomState *room = &g_rooms[i];
        // Check membership first
        int was_member = 0;
        for (int j = 0; j < room->member_count; j++) {
            if (room->member_fds[j] == client_fd) {
                was_member = 1;
                break;
            }
        }
        if (!was_member) continue;

        // Remove member
        room_remove_member((int)room->id, client_fd);

        // Broadcast that player left
        const char *msg = "Player left the room";
        room_broadcast((int)room->id, NTF_PLAYER_LEFT, msg, (uint32_t)strlen(msg), -1);
    }
}

void room_broadcast(int room_id, uint16_t command, const char *payload,
                   uint32_t payload_len, int exclude_fd) {
    RoomState *room = find_room((uint32_t)room_id);
    if (!room) {
        printf("[ROOM] Cannot broadcast to room %d - not found\\n", room_id);
        return;
    }
    
    // Build message header
    MessageHeader header;
    memset(&header, 0, sizeof(header));
    header.magic = htons(MAGIC_NUMBER);
    header.version = PROTOCOL_VERSION;
    header.command = htons(command);
    header.seq_num = 0; // Notifications don't need seq
    header.length = htonl(payload_len);
    
    // Send to all members
    int sent_count = 0;
    for (int i = 0; i < room->member_count; i++) {
        int fd = room->member_fds[i];
        
        // Skip excluded FD
        if (fd == exclude_fd) {
            continue;
        }
        
        // Send header + payload
        send(fd, &header, sizeof(header), 0);
        if (payload_len > 0 && payload) {
            send(fd, payload, payload_len, 0);
        }
        sent_count++;
    }
    
    printf("[ROOM] Broadcast cmd=0x%04x to room=%d (%d recipients)\\n",
           command, room_id, sent_count);
}

const int* room_get_members(int room_id, int *out_count) {
    RoomState *room = find_room((uint32_t)room_id);
    if (!room) {
        *out_count = 0;
        return NULL;
    }
    
    *out_count = room->member_count;
    return room->member_fds;
}


/**
 * Broadcast NTF_PLAYER_LIST to all members in a room
 */
void broadcast_player_list(uint32_t room_id) {
    RoomState *room = find_room(room_id);
    if (!room) {
        printf("[ROOM] Cannot broadcast player list - room %u not found\n", room_id);
        return;
    }
    
    // Build JSON payload with player list
    // Format: {"members":[{"account_id":47,"name":"Player1","is_host":true,"is_ready":false},...]}
    char payload[2048];
    int offset = 0;
    
    offset += snprintf(payload + offset, sizeof(payload) - offset, "{\"members\":[");
    
    for (int i = 0; i < room->player_count; i++) {
        RoomPlayerState *p = &room->players[i];
        
        if (i > 0) {
            offset += snprintf(payload + offset, sizeof(payload) - offset, ",");
        }
        
        offset += snprintf(payload + offset, sizeof(payload) - offset,
            "{\"account_id\":%u,\"name\":\"%s\",\"is_host\":%s,\"is_ready\":%s}",
            p->account_id,
            p->name,
            p->is_host ? "true" : "false",
            p->is_ready ? "true" : "false"
        );
    }
    
    offset += snprintf(payload + offset, sizeof(payload) - offset, "]}");
    
    printf("[ROOM] Broadcasting player list for room %u: %s\n", room_id, payload);
    
    // Broadcast to all members
    room_broadcast((int)room_id, NTF_PLAYER_LIST, payload, (uint32_t)offset, -1);
}

int room_get_count(void) {
    return g_room_count;
}

