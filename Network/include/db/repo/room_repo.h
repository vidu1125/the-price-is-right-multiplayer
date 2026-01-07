#pragma once
#include <stdint.h>
#include <stddef.h>

int room_repo_create(
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
    uint8_t wager_enabled,
    uint8_t visibility,
    char *out_buf,
    size_t out_size
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
