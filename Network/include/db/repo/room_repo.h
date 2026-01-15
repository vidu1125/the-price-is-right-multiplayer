#pragma once
#include <stdint.h>
#include <stddef.h>

int room_repo_create(
    uint32_t account_id,
    const char *name,
    uint8_t visibility,
    uint8_t mode,
    uint8_t max_players,
    uint8_t wager_enabled,
    char *out_buf,
    size_t out_size,
    uint32_t *room_id
);

int room_repo_close(uint32_t room_id, char *out_buf, size_t out_size);

int room_repo_set_rules(
    uint32_t room_id,
    uint8_t mode,
    uint8_t max_players,
    uint8_t visibility,
    uint8_t wager_enabled
);

int room_repo_kick_member(
    uint32_t room_id,
    uint32_t target_id,
    char *out_buf,
    size_t out_size
);

int room_repo_leave(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
);

//==============================================================================
// LEAVE ROOM OPERATIONS
//==============================================================================

/**
 * Remove player from room (DB only)
 * @param room_id Room ID
 * @param account_id Player account ID
 * @return 0 on success, -1 on error
 */
int room_repo_remove_player(uint32_t room_id, uint32_t account_id);

/**
 * Update room host (DB only)
 * @param room_id Room ID
 * @param new_host_id New host account ID
 * @return 0 on success, -1 on error
 */
int room_repo_update_host(uint32_t room_id, uint32_t new_host_id);

/**
 * Close room (set status = 'closed')
 * @param room_id Room ID
 * @return 0 on success, -1 on error
 */
int room_repo_close_room(uint32_t room_id);
