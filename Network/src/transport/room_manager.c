#include "transport/room_manager.h"
#include "protocol/protocol.h"
#include "protocol/opcode.h"
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//==============================================================================
// Global State
//==============================================================================

static RoomState g_rooms[MAX_ROOMS];
static int g_room_count = 0;

//==============================================================================
// Internal Helpers
//==============================================================================

static RoomState* find_room(int room_id) {
    for (int i = 0; i < g_room_count; i++) {
        if (g_rooms[i].room_id == room_id) {
            return &g_rooms[i];
        }
    }
    return NULL;
}

static RoomState* create_room(int room_id) {
    if (g_room_count >= MAX_ROOMS) {
        return NULL;
    }
    
    RoomState *room = &g_rooms[g_room_count++];
    room->room_id = room_id;
    room->member_count = 0;
    memset(room->members, 0, sizeof(room->members));
    
    return room;
}

//==============================================================================
// Public API
//==============================================================================

void room_add_member(int room_id, int client_fd, uint32_t account_id, bool is_host) {
    RoomState *room = find_room(room_id);
    
    // Create room if not exists
    if (!room) {
        room = create_room(room_id);
        if (!room) {
            printf("[ROOM] Cannot create room %d - max limit reached\n", room_id);
            return;
        }
    }
    
    // Check if already in room by account_id (not client_fd)
    for (int i = 0; i < room->member_count; i++) {
        if (room->members[i].account_id == account_id) {
            // Update FD if changed (reconnection case)
            if (room->members[i].client_fd != client_fd) {
                printf("[ROOM] Updating fd %d -> %d for account_id=%u in room=%d\n",
                       room->members[i].client_fd, client_fd, account_id, room_id);
                room->members[i].client_fd = client_fd;
            }
            return; // Already member
        }
    }
    
    // Add new member
    if (room->member_count < MAX_ROOM_MEMBERS) {
        RoomMember *member = &room->members[room->member_count++];
        member->client_fd = client_fd;
        member->account_id = account_id;
        member->is_ready = is_host; // ✅ Host auto ready
        
        printf("[ROOM] Added fd=%d (account_id=%u, ready=%d) to room=%d (%d members)\n", 
               client_fd, account_id, member->is_ready, room_id, room->member_count);
    } else {
        printf("[ROOM] Room %d is full\n", room_id);
    }
}

void room_remove_member(int room_id, int client_fd) {
    RoomState *room = find_room(room_id);
    if (!room) return;
    
    // Find and remove
    for (int i = 0; i < room->member_count; i++) {
        if (room->members[i].client_fd == client_fd) {
            // Shift remaining members
            for (int j = i; j < room->member_count - 1; j++) {
                room->members[j] = room->members[j + 1];
            }
            room->member_count--;
            printf("[ROOM] Removed fd=%d from room=%d (%d members left)\n",
                   client_fd, room_id, room->member_count);
            
            // Remove room if empty
            if (room->member_count == 0) {
                printf("[ROOM] Room %d is now empty, removing\n", room_id);
                // Shift remaining rooms
                for (int k = (int)(room - g_rooms); k < g_room_count - 1; k++) {
                    g_rooms[k] = g_rooms[k + 1];
                }
                g_room_count--;
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
        room_remove_member(room->room_id, client_fd);

        // Broadcast that player left
        const char *msg = "Player left the room";
        room_broadcast(room->room_id, NTF_PLAYER_LEFT, msg, (uint32_t)strlen(msg), -1);
    }
}

void room_set_ready(int room_id, int client_fd, bool ready) {
    RoomState *room = find_room(room_id);
    if (!room) {
        printf("[ROOM] Cannot set ready - room %d not found\n", room_id);
        return;
    }
    
    for (int i = 0; i < room->member_count; i++) {
        if (room->members[i].client_fd == client_fd) {
            room->members[i].is_ready = ready;
            printf("[ROOM] Member fd=%d set ready=%d in room=%d\n", 
                   client_fd, ready, room_id);
            return;
        }
    }
    
    printf("[ROOM] Cannot set ready - fd=%d not found in room=%d\n", client_fd, room_id);
}

bool room_all_ready(int room_id, int *out_ready_count, int *out_total_count) {
    RoomState *room = find_room(room_id);
    if (!room) {
        *out_ready_count = 0;
        *out_total_count = 0;
        return false;
    }
    
    int ready_count = 0;
    for (int i = 0; i < room->member_count; i++) {
        if (room->members[i].is_ready) {
            ready_count++;
        }
    }
    
    *out_ready_count = ready_count;
    *out_total_count = room->member_count;
    
    return (ready_count == room->member_count);
}

int room_get_member_count(int room_id) {
    RoomState *room = find_room(room_id);
    return room ? room->member_count : 0;
}

void room_broadcast(int room_id, uint16_t command, const char *payload,
                   uint32_t payload_len, int exclude_fd) {
    RoomState *room = find_room(room_id);
    if (!room) {
        printf("[ROOM] Cannot broadcast to room %d - not found\n", room_id);
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
        int fd = room->members[i].client_fd;
        
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
    
    printf("[ROOM] Broadcast cmd=0x%04x to room=%d (%d recipients)\n",
           command, room_id, sent_count);
}

const int* room_get_members(int room_id, int *out_count) {
    RoomState *room = find_room(room_id);
    if (!room) {
        *out_count = 0;
        return NULL;
    }
    
    // Build temporary array of FDs
    static int fd_array[MAX_ROOM_MEMBERS];
    for (int i = 0; i < room->member_count; i++) {
        fd_array[i] = room->members[i].client_fd;
    }
    
    *out_count = room->member_count;
    return fd_array;
}

int room_get_count(void) {
    return g_room_count;
}