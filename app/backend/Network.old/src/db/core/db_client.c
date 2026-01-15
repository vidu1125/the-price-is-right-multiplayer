#include "db/core/db_client.h"
#include "db/core/db_config.h"
#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

static PGconn *g_db_conn = NULL;

/* ===============================
 * Init / Cleanup
 * =============================== */
db_error_t db_client_init(void) {
    const char *host = getenv(ENV_DB_HOST);
    const char *port = getenv(ENV_DB_PORT);
    const char *dbname = getenv(ENV_DB_NAME);
    const char *user = getenv(ENV_DB_USER);
    const char *password = getenv(ENV_DB_PASSWORD);

    // Default values if not set
    if (!host) host = "localhost";
    if (!port) port = "5432";
    if (!dbname) dbname = "tpir";
    if (!user) user = "postgresql";
    if (!password) password = "password";

    printf("[DB_CLIENT] Init: host=%s, port=%s, dbname=%s, user=%s\n",
           host, port, dbname, user);

    char conninfo[DB_MAX_CONN_STRING];
    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%s dbname=%s user=%s password=%s connect_timeout=%d",
             host, port, dbname, user, password, DB_CONN_TIMEOUT_SEC);

    g_db_conn = PQconnectdb(conninfo);

    if (PQstatus(g_db_conn) != CONNECTION_OK) {
        fprintf(stderr, "[DB_CLIENT] Connection failed: %s\n", 
                PQerrorMessage(g_db_conn));
        PQfinish(g_db_conn);
        g_db_conn = NULL;
        return DB_ERR_HTTP;
    }

    printf("[DB_CLIENT] PostgreSQL connection established\n");
    return DB_OK;
}

void db_client_cleanup(void) {
    if (g_db_conn) {
        PQfinish(g_db_conn);
        g_db_conn = NULL;
        printf("[DB_CLIENT] PostgreSQL connection closed\n");
    }
}

/* ===============================
 * Helper: Convert PGresult to cJSON
 * =============================== */
static cJSON *pgresult_to_json(PGresult *res) {
    if (!res) return NULL;

    int nrows = PQntuples(res);
    int nfields = PQnfields(res);

    cJSON *array = cJSON_CreateArray();
    if (!array) return NULL;

    for (int row = 0; row < nrows; row++) {
        cJSON *obj = cJSON_CreateObject();
        if (!obj) {
            cJSON_Delete(array);
            return NULL;
        }

        for (int col = 0; col < nfields; col++) {
            const char *field_name = PQfname(res, col);
            const char *value = PQgetvalue(res, row, col);
            
            if (PQgetisnull(res, row, col)) {
                cJSON_AddNullToObject(obj, field_name);
            } else {
                // Try to detect type (simplified version)
                Oid field_type = PQftype(res, col);
                
                // Common PostgreSQL type OIDs
                // 16=bool, 20=int8, 21=int2, 23=int4, 25=text, 1043=varchar
                // 114=json, 3802=jsonb
                if (field_type == 16) { // boolean
                    cJSON_AddBoolToObject(obj, field_name, value[0] == 't');
                } else if (field_type == 20 || field_type == 21 || field_type == 23) { // integers
                    cJSON_AddNumberToObject(obj, field_name, atoi(value));
                } else if (field_type == 114 || field_type == 3802) { // json/jsonb
                    cJSON *json_val = cJSON_Parse(value);
                    if (json_val) {
                        cJSON_AddItemToObject(obj, field_name, json_val);
                    } else {
                        cJSON_AddStringToObject(obj, field_name, value);
                    }
                } else { // default to string
                    cJSON_AddStringToObject(obj, field_name, value);
                }
            }
        }

        cJSON_AddItemToArray(array, obj);
    }

    return array;
}

/* ===============================
 * Execute SQL query
 * =============================== */
static db_error_t db_exec(const char *query, cJSON **out_json) {
    if (!g_db_conn) {
        fprintf(stderr, "[DB_EXEC] No database connection\n");
        return DB_ERR_HTTP;
    }

    printf("[DB_EXEC] Query: %.200s%s\n", query, strlen(query) > 200 ? "..." : "");

    PGresult *res = PQexec(g_db_conn, query);
    if (!res) {
        fprintf(stderr, "[DB_EXEC] Query failed: %s\n", PQerrorMessage(g_db_conn));
        return DB_ERR_HTTP;
    }

    ExecStatusType status = PQresultStatus(res);

    if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DB_EXEC] Query error: %s\n", PQerrorMessage(g_db_conn));
        PQclear(res);
        return DB_ERR_HTTP;
    }

    if (out_json) {
        *out_json = pgresult_to_json(res);
        if (*out_json) {
            char *json_str = cJSON_PrintUnformatted(*out_json);
            printf("[DB_EXEC] Result: %.200s%s\n", json_str, 
                   strlen(json_str) > 200 ? "..." : "");
            free(json_str);
        }
    }

    PQclear(res);
    return DB_OK;
}

/* ===============================
 * Public API - Now using SQL
 * =============================== */

// GET: Execute SQL SELECT query  
db_error_t db_get(const char *table, const char *query, cJSON **out_json) {
    // Query now contains full SQL statement
    // table parameter kept for API compatibility but not used in SQL mode
    (void)table; // Unused in SQL mode
    
    return db_exec(query, out_json);
}

// POST: INSERT and return inserted row
db_error_t db_post(const char *table, cJSON *payload, cJSON **out_json) {
    if (!payload) return DB_ERR_INVALID_ARG;

    // Build INSERT statement
    char sql[4096];
    char columns[1024] = "";
    char values[2048] = "";
    
    cJSON *item = payload->child;
    int first = 1;
    
    while (item) {
        if (!first) {
            strcat(columns, ", ");
            strcat(values, ", ");
        }
        
        strcat(columns, item->string);
        
        if (cJSON_IsString(item)) {
            // Escape single quotes (simple version)
            char escaped[512];
            const char *str = item->valuestring;
            int j = 0;
            for (size_t i = 0; i < strlen(str) && j < 510; i++) {
                if (str[i] == '\'') {
                    escaped[j++] = '\'';
                }
                escaped[j++] = str[i];
            }
            escaped[j] = '\0';
            
            strcat(values, "'");
            strcat(values, escaped);
            strcat(values, "'");
        } else if (cJSON_IsNumber(item)) {
            char num[32];
            snprintf(num, sizeof(num), "%d", item->valueint);
            strcat(values, num);
        } else if (cJSON_IsBool(item)) {
            // Check cJSON type, not valueint (more reliable)
            int is_true = (item->type == cJSON_True);
            strcat(values, is_true ? "TRUE" : "FALSE");
        } else if (cJSON_IsNull(item)) {
            strcat(values, "NULL");
        } else if (cJSON_IsObject(item) || cJSON_IsArray(item)) {
            char *json_str = cJSON_PrintUnformatted(item);
            strcat(values, "'");
            strcat(values, json_str);
            strcat(values, "'::jsonb");
            free(json_str);
        }
        
        first = 0;
        item = item->next;
    }
    
    snprintf(sql, sizeof(sql), 
             "INSERT INTO %s (%s) VALUES (%s) RETURNING *",
             table, columns, values);
    
    return db_exec(sql, out_json);
}

// RPC: Call PostgreSQL function
db_error_t db_rpc(const char *function, cJSON *payload, cJSON **out_json) {
    char sql[2048];
    
    // Build function call
    // Format: SELECT * FROM function_name(param1, param2, ...)
    
    char params[1024] = "";
    if (payload) {
        cJSON *item = payload->child;
        int first = 1;
        
        while (item) {
            if (!first) strcat(params, ", ");
            
            if (cJSON_IsString(item)) {
                strcat(params, "'");
                strcat(params, item->valuestring);
                strcat(params, "'");
            } else if (cJSON_IsNumber(item)) {
                char num[32];
                snprintf(num, sizeof(num), "%d", item->valueint);
                strcat(params, num);
            }
            
            first = 0;
            item = item->next;
        }
    }
    
    snprintf(sql, sizeof(sql), "SELECT * FROM %s(%s)", function, params);
    
    return db_exec(sql, out_json);
}

// PATCH: UPDATE and return updated rows
db_error_t db_patch(const char *table, const char *filter, cJSON *payload, cJSON **out_json) {
    if (!payload ||!filter) return DB_ERR_INVALID_ARG;

    char sql[4096];
    char set_clause[2048] = "";
    
    cJSON *item = payload->child;
    int first = 1;
    
    while (item) {
        if (!first) strcat(set_clause, ", ");
        
        strcat(set_clause, item->string);
        strcat(set_clause, " = ");
        
        if (cJSON_IsString(item)) {
            strcat(set_clause, "'");
            strcat(set_clause, item->valuestring);
            strcat(set_clause, "'");
        } else if (cJSON_IsNumber(item)) {
            char num[32];
            snprintf(num, sizeof(num), "%d", item->valueint);
            strcat(set_clause, num);
        } else if (cJSON_IsBool(item)) {
            // Check cJSON type, not valueint
            int is_true = (item->type == cJSON_True);
            strcat(set_clause, is_true ? "TRUE" : "FALSE");
        }
        
        first = 0;
        item = item->next;
    }
    
    // TODO: Parse filter properly (e.g., "id=eq.5" -> "id = 5")
    // For now, simplified
    
    snprintf(sql, sizeof(sql), 
             "UPDATE %s SET %s WHERE %s RETURNING *",
             table, set_clause, filter);
    
    return db_exec(sql, out_json);
}

// DELETE: Delete rows
db_error_t db_delete(const char *table, const char *filter, cJSON **out_json) {
    if (!filter) return DB_ERR_INVALID_ARG;

    char sql[1024];
    
    // TODO: Parse filter properly
    snprintf(sql, sizeof(sql), 
             "DELETE FROM %s WHERE %s RETURNING *",
             table, filter);
    
    return db_exec(sql, out_json);
}

// Ping: Test connection
int db_ping(void) {
    if (!g_db_conn) {
        printf("[DB] ping failed: no connection\n");
        return -1;
    }

    PGresult *res = PQexec(g_db_conn, "SELECT 1");
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("[DB] ping failed\n");
        if (res) PQclear(res);
        return -1;
    }

    printf("[DB] ping OK\n");
    PQclear(res);
    return 0;
}
