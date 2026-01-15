#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <cjson/cJSON.h>

#include "handlers/presence_manager.h"
#include "db/repo/friend_repo.h"
#include "protocol/protocol.h"
#include "protocol/opcode.h"

#define MAX_ONLINE_USERS 1024

static online_user_t g_online_users[MAX_ONLINE_USERS];
static int32_t g_online_count = 0;

// ============================================================================
// HELPERS
// ============================================================================

static online_user_t* find_online_user(int32_t account_id) {
    for (int32_t i = 0; i < g_online_count; i++) {
        if (g_online_users[i].account_id == account_id) {
            return &g_online_users[i];
        }
    }
    return NULL;
}

static online_user_t* allocate_online_user(void) {
    if (g_online_count >= MAX_ONLINE_USERS) {
        printf("[PRESENCE] ERROR: Max online users reached!\n");
        return NULL;
    }
    return &g_online_users[g_online_count++];
}

static void notify_friend_status(
    int32_t friend_fd,
    int32_t account_id,
    presence_status_t status,
    int32_t room_id
) {
    if (friend_fd <= 0) return;

    // Prepare status change notification
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "friend_id", account_id);
    cJSON_AddNumberToObject(payload, "status", (int)status);
    if (room_id > 0) {
        cJSON_AddNumberToObject(payload, "room_id", room_id);
    }

    char *json_str = cJSON_PrintUnformatted(payload);
    if (json_str) {
        // Create a simple response header
        MessageHeader header = {
            .magic = MAGIC_NUMBER,
            .version = PROTOCOL_VERSION,
            .flags = 0,
            .command = NTF_FRIEND_STATUS,
            .reserved = 0,
            .seq_num = 0,
            .length = (uint32_t)strlen(json_str)
        };

        // Send notification
        forward_response(
            friend_fd,
            &header,
            NTF_FRIEND_STATUS,
            json_str,
            strlen(json_str)
        );

        free(json_str);
    }

    cJSON_Delete(payload);
}

// ============================================================================
// PUBLIC API
// ============================================================================

void presence_manager_init(void) {
    printf("[PRESENCE] Initializing presence manager\n");
    memset(g_online_users, 0, sizeof(g_online_users));
    g_online_count = 0;
}

void presence_register_online(
    int32_t account_id,
    int32_t client_fd,
    const char *username,
    const char *avatar
) {
    printf("[PRESENCE] Registering user %d (fd=%d) as online\n", account_id, client_fd);

    if (account_id <= 0 || client_fd <= 0) {
        printf("[PRESENCE] ERROR: Invalid account_id or client_fd\n");
        return;
    }

    // Check if already online
    online_user_t *existing = find_online_user(account_id);
    if (existing) {
        printf("[PRESENCE] User already online, updating fd: %d -> %d\n", existing->client_fd, client_fd);
        existing->client_fd = client_fd;
        existing->status = PRESENCE_ONLINE_IDLE;
        existing->last_heartbeat = time(NULL);
        return;
    }

    // Allocate new entry
    online_user_t *user = allocate_online_user();
    if (!user) {
        return;
    }

    user->account_id = account_id;
    user->client_fd = client_fd;
    user->status = PRESENCE_ONLINE_IDLE;
    user->current_room_id = 0;
    user->last_heartbeat = time(NULL);

    // Load friend list from DB
    int32_t *friend_ids = NULL;
    int32_t friend_count = 0;

    db_error_t err = friend_list_get_ids(account_id, &friend_ids, &friend_count);
    if (err == DB_SUCCESS && friend_count > 0) {
        user->friend_ids = friend_ids;
        user->friend_count = friend_count;
        printf("[PRESENCE] Loaded %d friends for user %d\n", friend_count, account_id);
    } else {
        user->friend_ids = NULL;
        user->friend_count = 0;
        printf("[PRESENCE] User %d has no friends\n", account_id);
    }

    printf("[PRESENCE] User %d now online (total: %d)\n", account_id, g_online_count);

    // Notify all online friends
    presence_broadcast_status_change(account_id, PRESENCE_ONLINE_IDLE, 0);
}

void presence_update_status(
    int32_t account_id,
    presence_status_t status,
    int32_t room_id
) {
    printf("[PRESENCE] Updating status for user %d: status=%d, room=%d\n", account_id, status, room_id);

    online_user_t *user = find_online_user(account_id);
    if (!user) {
        printf("[PRESENCE] ERROR: User %d not found in online list\n", account_id);
        return;
    }

    if (user->status == status && user->current_room_id == room_id) {
        return; // No change
    }

    user->status = status;
    user->current_room_id = room_id;
    user->last_heartbeat = time(NULL);

    // Broadcast to all online friends
    presence_broadcast_status_change(account_id, status, room_id);
}

void presence_unregister_offline(int32_t account_id) {
    printf("[PRESENCE] Unregistering user %d as offline\n", account_id);

    online_user_t *user = find_online_user(account_id);
    if (!user) {
        printf("[PRESENCE] User %d not found in online list\n", account_id);
        return;
    }

    // Notify all online friends before removing
    presence_broadcast_status_change(account_id, PRESENCE_OFFLINE, 0);

    // Free friend list
    if (user->friend_ids) {
        free(user->friend_ids);
        user->friend_ids = NULL;
    }

    // Remove from array by shifting
    for (int32_t i = 0; i < g_online_count - 1; i++) {
        if (g_online_users[i].account_id == account_id) {
            memmove(&g_online_users[i], &g_online_users[i + 1],
                    sizeof(online_user_t) * (g_online_count - i - 1));
            break;
        }
    }

    g_online_count--;
    printf("[PRESENCE] User %d now offline (total: %d)\n", account_id, g_online_count);
}

const online_user_t* presence_get_user(int32_t account_id) {
    return find_online_user(account_id);
}

online_user_t* presence_get_user_mut(int32_t account_id) {
    return find_online_user(account_id);
}

friend_presence_t* presence_get_friends_status(
    int32_t account_id,
    int32_t *out_count
) {
    if (!out_count) return NULL;

    online_user_t *user = find_online_user(account_id);
    if (!user || user->friend_count == 0) {
        *out_count = 0;
        return NULL;
    }

    // Allocate result array
    friend_presence_t *result = calloc(user->friend_count, sizeof(friend_presence_t));
    if (!result) {
        *out_count = 0;
        return NULL;
    }

    int32_t filled = 0;
    for (int32_t i = 0; i < user->friend_count; i++) {
        int32_t friend_id = user->friend_ids[i];
        online_user_t *friend = find_online_user(friend_id);

        result[filled].friend_id = friend_id;
        
        if (friend) {
            result[filled].status = friend->status;
            result[filled].current_room_id = friend->current_room_id;
        } else {
            result[filled].status = PRESENCE_OFFLINE;
            result[filled].current_room_id = 0;
        }

        filled++;
    }

    *out_count = filled;
    return result;
}

bool presence_is_online(int32_t account_id) {
    return find_online_user(account_id) != NULL;
}

presence_status_t presence_get_status(int32_t account_id) {
    online_user_t *user = find_online_user(account_id);
    if (!user) {
        return PRESENCE_OFFLINE;
    }
    return user->status;
}

void presence_broadcast_status_change(
    int32_t account_id,
    presence_status_t new_status,
    int32_t room_id
) {
    printf("[PRESENCE] Broadcasting status change for user %d\n", account_id);

    // Find the user to get their friend list
    online_user_t *user = find_online_user(account_id);
    if (!user) {
        printf("[PRESENCE] User %d not found (probably offline), skipping broadcast\n", account_id);
        return;
    }

    // Notify each online friend
    for (int32_t i = 0; i < user->friend_count; i++) {
        int32_t friend_id = user->friend_ids[i];
        online_user_t *friend = find_online_user(friend_id);

        if (friend && friend->client_fd > 0) {
            printf("[PRESENCE] Notifying friend %d about status change\n", friend_id);
            notify_friend_status(friend->client_fd, account_id, new_status, room_id);
        }
    }
}

void presence_cleanup(void) {
    printf("[PRESENCE] Cleaning up presence manager\n");
    
    for (int32_t i = 0; i < g_online_count; i++) {
        if (g_online_users[i].friend_ids) {
            free(g_online_users[i].friend_ids);
            g_online_users[i].friend_ids = NULL;
        }
    }

    memset(g_online_users, 0, sizeof(g_online_users));
    g_online_count = 0;
}
