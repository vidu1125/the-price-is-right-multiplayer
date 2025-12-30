#pragma once
#include <stdint.h>
#include <stddef.h>

// START GAME / MATCH
int match_repo_start(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
);
