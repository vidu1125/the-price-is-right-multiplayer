#ifndef DB_CLIENT_H
#define DB_CLIENT_H

#include "db_error.h"

/*
 * DB Client (Supabase REST)
 *
 * - Blocking HTTP (libcurl)
 * - Thread-safe (stateless)
 * - MUST NOT be called inside realtime recv loop
 */
typedef struct cJSON cJSON;   // forward declaration

/* init / cleanup */
db_error_t db_client_init(void);
void db_client_cleanup(void);

/* GET /rest/v1/{table}?{query} */
db_error_t db_get(
    const char *table,
    const char *query,
    cJSON **out_json
);

/* POST /rest/v1/{table} */
db_error_t db_post(
    const char *table,
    cJSON *payload,
    cJSON **out_json
);

/* POST /rest/v1/rpc/{function} */
db_error_t db_rpc(
    const char *function,
    cJSON *payload,
    cJSON **out_json
);

db_error_t db_patch(
    const char *table,
    const char *filter,
    cJSON *payload,
    cJSON **out_json
);

/* DELETE /rest/v1/{table}?{filter} */
db_error_t db_delete(
    const char *table,
    const char *filter,
    cJSON **out_json
);

int db_ping(void);

#endif
