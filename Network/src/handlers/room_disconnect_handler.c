#include "handlers/room_disconnect_handler.h"
#include "transport/room_manager.h"
#include "db/core/db_client.h"
#include "protocol/opcode.h"
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

void room_handle_disconnect(int client_fd, uint32_t account_id) {
    printf("========================================\n");
    printf("[ROOM_DISCONNECT] START\n");
    printf("[ROOM_DISCONNECT] client_fd=%d, account_id=%u\n", client_fd, account_id);
    
    // 1. Find which room this player is in
    uint32_t room_id = room_find_by_player_fd(client_fd);
    if (room_id == 0) {
        printf("[ROOM_DISCONNECT] Player not in any room\n");
        printf("[ROOM_DISCONNECT] END (no cleanup needed)\n");
        printf("========================================\n");
        return;
    }
    
    printf("[ROOM_DISCONNECT] Found player in room_id=%u\n", room_id);
    
    // Get room state BEFORE removal
    RoomState *room_before = room_get_state(room_id);
    if (room_before) {
        printf("[ROOM_DISCONNECT] Room state BEFORE removal:\n");
        printf("  - room_id: %u\n", room_before->id);
        printf("  - room_name: %s\n", room_before->name);
        printf("  - player_count: %d\n", room_before->player_count);
        printf("  - member_count: %d\n", room_before->member_count);
        printf("  - host_id: %u\n", room_before->host_id);
    }
    
    // 2. Remove from in-memory RoomState
    printf("[ROOM_DISCONNECT] Removing player from in-memory RoomState...\n");
    room_remove_member(room_id, client_fd);
    
    // 3. Delete from database room_members table
    printf("[ROOM_DISCONNECT] Deleting from database room_members...\n");
    char query[256];
    snprintf(query, sizeof(query), 
             "account_id=eq.%u&room_id=eq.%u", account_id, room_id);
    
    cJSON *response = NULL;
    db_error_t rc = db_delete("room_members", query, &response);
    
    if (rc == DB_OK) {
        printf("[ROOM_DISCONNECT] ✓ Deleted from room_members table\n");
    } else {
        printf("[ROOM_DISCONNECT] ✗ Failed to delete from room_members: rc=%d\n", rc);
    }
    
    if (response) cJSON_Delete(response);
    
    // 4. Check if room is now empty
    RoomState *room = room_get_state(room_id);
    
    if (room && room->player_count == 0) {
        // Room is empty - close it
        printf("[ROOM_DISCONNECT] ⚠️  ROOM IS EMPTY - CLOSING\n");
        printf("[ROOM_DISCONNECT] Room %u has 0 players, closing...\n", room_id);
        
        // Update database: set status to 'closed'
        cJSON *payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "status", "closed");
        
        char filter[64];
        snprintf(filter, sizeof(filter), "id=eq.%u", room_id);
        
        response = NULL;
        rc = db_patch("rooms", filter, payload, &response);
        
        if (rc == DB_OK) {
            printf("[ROOM_DISCONNECT] ✓ Room %u status set to 'closed' in DB\n", room_id);
        } else {
            printf("[ROOM_DISCONNECT] ✗ Failed to close room in DB: rc=%d\n", rc);
        }
        
        cJSON_Delete(payload);
        if (response) cJSON_Delete(response);
        
        // Destroy in-memory room state
        printf("[ROOM_DISCONNECT] Destroying in-memory room state...\n");
        room_destroy(room_id);
        printf("[ROOM_DISCONNECT] ✓ Room %u destroyed\n", room_id);
        
    } else if (room) {
        // Room still has players - broadcast notification
        printf("[ROOM_DISCONNECT] Room %u still has %d players remaining\n", 
               room_id, room->player_count);
        printf("[ROOM_DISCONNECT] Broadcasting NTF_PLAYER_LEFT to remaining players...\n");
        
        // Broadcast NTF_PLAYER_LEFT to remaining players
        char notif[256];
        snprintf(notif, sizeof(notif), 
                 "{\"account_id\":%u,\"room_id\":%u}", account_id, room_id);
        
        room_broadcast(room_id, NTF_PLAYER_LEFT, notif, strlen(notif), -1);
        printf("[ROOM_DISCONNECT] ✓ Notification sent\n");
    } else {
        printf("[ROOM_DISCONNECT] ⚠️  Room state not found after removal\n");
    }
    
    printf("[ROOM_DISCONNECT] END\n");
    printf("========================================\n");
}
