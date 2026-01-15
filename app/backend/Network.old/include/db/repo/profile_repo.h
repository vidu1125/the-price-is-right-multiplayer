#ifndef PROFILE_REPO_H
#define PROFILE_REPO_H

#include "db/models/model.h"
#include "db/core/db_error.h"
#include <stdbool.h>

// Create profile with optional fields; matches/wins/points default to 0
// Caller must free out_profile via profile_free.
db_error_t profile_create(
    int32_t account_id,
    const char *name,
    const char *avatar,
    const char *bio,
    profile_t **out_profile
);

// Find profile by owning account id; caller frees.
db_error_t profile_find_by_account(
    int32_t account_id,
    profile_t **out_profile
);

void profile_free(profile_t *profile);

// Update profile fields (name, avatar, bio) by owning account
db_error_t profile_update_by_account(
    int32_t account_id,
    const char *name,
    const char *avatar,
    const char *bio,
    profile_t **out_profile
);

#endif // PROFILE_REPO_H
