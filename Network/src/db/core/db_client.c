#include "db/core/db_client.h"
#include "db/core/db_config.h"
#include "db/repo/room_repo.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

static const char *g_supabase_url = NULL;
static const char *g_supabase_key = NULL;

/* ===============================
 * HTTP buffer
 * =============================== */
typedef struct {
    char *data;
    size_t size;
} http_buf_t;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    http_buf_t *buf = userdata;

    char *tmp = realloc(buf->data, buf->size + total + 1);
    if (!tmp) return 0;

    buf->data = tmp;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';

    return total;
}

/* ===============================
 * Init / Cleanup
 * =============================== */
db_error_t db_client_init(void) {
    g_supabase_url = getenv(ENV_SUPABASE_URL);
    g_supabase_key = getenv(ENV_SUPABASE_KEY);

    if (!g_supabase_url || !g_supabase_key)
        return DB_ERR_INVALID_ARG;

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
        return DB_ERR_HTTP;

    return DB_OK;
}

void db_client_cleanup(void) {
    curl_global_cleanup();
}

/* ===============================
 * HTTP helper
 * =============================== */
static db_error_t http_request(
    const char *method,
    const char *url,
    const char *body,
    cJSON **out_json
) {
    CURL *curl = curl_easy_init();
    if (!curl) return DB_ERR_HTTP;

    http_buf_t buf = {0};

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Prefer: return=representation");

    char h[512];
    snprintf(h, sizeof(h), "apikey: %s", g_supabase_key);
    headers = curl_slist_append(headers, h);

    snprintf(h, sizeof(h), "Authorization: Bearer %s", g_supabase_key);
    headers = curl_slist_append(headers, h);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, DB_HTTP_TIMEOUT_SEC);

    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    }

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(buf.data);
        return DB_ERR_HTTP;
    }

    if (http_code == 401) return DB_ERR_UNAUTHORIZED;
    if (http_code == 403) return DB_ERR_FORBIDDEN;
    if (http_code >= 400) return DB_ERR_HTTP;

    if (!buf.data) return DB_ERR_EMPTY;

    *out_json = cJSON_Parse(buf.data);
    free(buf.data);

    return (*out_json) ? DB_OK : DB_ERR_PARSE;
}

/* ===============================
 * Public API
 * =============================== */
db_error_t db_get(const char *table, const char *query, cJSON **out_json) {
    char url[DB_HTTP_MAX_URL];
    snprintf(url, sizeof(url), "%s%s/%s?%s",
             g_supabase_url, SUPABASE_REST_PATH, table, query);
    return http_request("GET", url, NULL, out_json);
}

// int db_ping(void) {
//     cJSON *json = NULL;

//     // 🔹 chỉ test nhẹ: lấy 1 room
//     db_error_t rc = db_get(
//         "rooms",
//         "select=id&limit=1",
//         &json
//     );

//     if (rc == DB_OK && json) {
//         // optional: log cho debug
//         printf("[DB] ping OK\n");

//         cJSON_Delete(json);
//         return 0;
//     }

//     printf("[DB] ping failed rc=%d\n", rc);
//     return -1;
// }

int db_ping(void) {
    cJSON *json = NULL;

    db_error_t rc = db_get("rooms", "select=id,code&limit=1", &json);

    if (rc != DB_OK || !json) {
        printf("[DB] ping failed rc=%d\n", rc);
        return -1;
    }

    if (!cJSON_IsArray(json) || cJSON_GetArraySize(json) == 0) {
        printf("[DB] ping OK but no rooms\n");
        cJSON_Delete(json);
        return 0;
    }

    printf("[DB] ping OK, rooms sample found\n");
    cJSON_Delete(json);
    return 0;
}
