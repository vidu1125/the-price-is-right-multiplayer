#pragma once
#include <stddef.h>
#include <cjson/cJSON.h>
#include <stdint.h> 
#include "db/core/db_client.h"  
// GET HISTORY
int history_repo_get(
    int32_t account_id,
    cJSON **out_json
);
