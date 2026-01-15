#include "handlers/room_disconnect_handler.h"
#include "transport/room_manager.h"
#include "db/core/db_client.h"
#include "protocol/opcode.h"
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <limits.h>
#include <stdlib.h>

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
             "account_id = %u AND room_id = %u", account_id, room_id);
    
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
        snprintf(filter, sizeof(filter), "id = %u", room_id);
        
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
        // Room still has players - check for host transfer
        printf("[ROOM_DISCONNECT] Room %u still has %d players remaining\n", 
               room_id, room->player_count);
        
        // Check if disconnected player was the host
        bool was_host = (room_before && room_before->host_id == account_id);
        uint32_t new_host_id = 0;
        
        if (was_host) {
            printf("[ROOM_DISCONNECT] Host disconnected! Finding new host...\n");
            
            // Find earliest connected joiner
            time_t earliest_join = LONG_MAX;
            for (int i = 0; i < room->player_count; i++) {
                if (room->players[i].connected && room->players[i].joined_at < earliest_join) {
                    earliest_join = room->players[i].joined_at;
                    new_host_id = room->players[i].account_id;
                }
            }
            
            if (new_host_id != 0) {
                printf("[ROOM_DISCONNECT] New host selected: %u\n", new_host_id);
                
                // Update in-memory
                room->host_id = new_host_id;
                for (int i = 0; i < room->player_count; i++) {
                    room->players[i].is_host = (room->players[i].account_id == new_host_id);
                }
                
                // Update DB
                char filter[64];
                snprintf(filter, sizeof(filter), "id = %u", room_id);
                
                cJSON *host_payload = cJSON_CreateObject();
                cJSON_AddNumberToObject(host_payload, "host_id", new_host_id);
                
                cJSON *host_resp = NULL;
                db_error_t host_rc = db_patch("rooms", filter, host_payload, &host_resp);
                
                if (host_rc == DB_OK) {
                    printf("[ROOM_DISCONNECT] ✓ Host updated in DB\n");
                } else {
                    printf("[ROOM_DISCONNECT] ✗ Failed to update host in DB\n");
                }
                
                cJSON_Delete(host_payload);
                if (host_resp) cJSON_Delete(host_resp);
            }
        }
        
        printf("[ROOM_DISCONNECT] Broadcasting NTF_PLAYER_LEFT to remaining players...\n");
        
        // Broadcast NTF_PLAYER_LEFT to remaining players
        cJSON *left_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(left_json, "account_id", account_id);
        cJSON_AddStringToObject(left_json, "reason", "disconnected");
        char *left_str = cJSON_PrintUnformatted(left_json);
        
        room_broadcast(room_id, NTF_PLAYER_LEFT, left_str, strlen(left_str), -1);
        printf("[ROOM_DISCONNECT] ✓ NTF_PLAYER_LEFT sent\n");
        
        free(left_str);
        cJSON_Delete(left_json);
        
        // Broadcast NTF_HOST_CHANGED if host was transferred
        if (was_host && new_host_id != 0) {
            cJSON *host_json = cJSON_CreateObject();
            cJSON_AddNumberToObject(host_json, "new_host_id", new_host_id);
            char *host_str = cJSON_PrintUnformatted(host_json);
            
            printf("[ROOM_DISCONNECT] Broadcasting NTF_HOST_CHANGED: %s\n", host_str);
            room_broadcast(room_id, NTF_HOST_CHANGED, host_str, strlen(host_str), -1);
            
            free(host_str);
            cJSON_Delete(host_json);
        }
        
        // Broadcast NTF_PLAYER_LIST
        printf("[ROOM_DISCONNECT] Broadcasting NTF_PLAYER_LIST\n");
        broadcast_player_list(room_id);
        
    } else {
        printf("[ROOM_DISCONNECT] ⚠️  Room state not found after removal\n");
    }
    
    printf("[ROOM_DISCONNECT] END\n");
    printf("========================================\n");
}
