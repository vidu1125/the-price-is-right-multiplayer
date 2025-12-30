#include <stddef.h>
#include <string.h>
#include "db/repo/question_repo.h"

int history_repo_get(
    char *out_buf,
    size_t out_size
) {
    const char *json = "{\"history\":[]}";
    strncpy(out_buf, json, out_size - 1);
    out_buf[out_size - 1] = '\0';
    return 0;
}
