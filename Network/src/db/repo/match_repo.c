#include <stdint.h>   // uint32_t
#include <stddef.h>   // size_t
#include <stdio.h>    // snprintf
#include <string.h>   // memcpy / strlen nếu dùng

int match_repo_start(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
) {
    (void)room_id;

    snprintf(
        out_buf,
        out_size,
        "{\"success\":true,\"message\":\"game started\"}"
    );

    return 0;
}