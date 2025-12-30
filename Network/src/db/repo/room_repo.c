#include "db/repo/room_repo.h"
#include <stdio.h>
int room_repo_create(
    const char *name,
    uint8_t visibility,
    uint8_t mode,
    uint8_t max_players,
    uint8_t round_time,
    uint8_t wager_enabled,
    char *out_buf,
    size_t out_size,
    uint32_t *room_id
) {
    (void)visibility;
    (void)mode;
    (void)max_players;
    (void)round_time;
    (void)wager_enabled;

    *room_id = 1;

    const char *json = "{\"success\":true,\"room_id\":1}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';

    return 0;
}

int room_repo_close(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
) {
    (void)room_id;

    const char *json = "{\"success\":true}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';

    return 0;
}
int room_repo_set_rules(
    uint32_t room_id,
    uint8_t mode,
    uint8_t max_players,
    uint8_t round_time,
    uint8_t wager_enabled,
    char *out_buf,
    size_t out_size
) {
    (void)room_id;
    (void)mode;
    (void)max_players;
    (void)round_time;
    (void)wager_enabled;

    const char *json = "{\"success\":true}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';

    return 0;
}
int room_repo_kick_member(
    uint32_t room_id,
    uint32_t target_id,
    char *out_buf,
    size_t out_size
) {
    (void)room_id;
    (void)target_id;

    const char *json = "{\"success\":true}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';

    return 0;
}

int room_repo_leave(
    uint32_t room_id,
    char *out_buf,
    size_t out_size
) {
    (void)room_id;

    const char *json = "{\"success\":true}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';

    return 0;
}
