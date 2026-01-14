#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <cjson/cJSON.h>

#include "db/repo/friend_repo.h"
#include "db/core/db_client.h"

// ============================================================================
// PARSING HELPERS
// ============================================================================

static friend_request_t* parse_friend_request_from_json(cJSON *json) {
    if (!json) return NULL;

    friend_request_t *req = calloc(1, sizeof(friend_request_t));
    if (!req) return NULL;

    cJSON *id = cJSON_GetObjectItem(json, "id");
    cJSON *sender_id = cJSON_GetObjectItem(json, "sender_id");
    cJSON *receiver_id = cJSON_GetObjectItem(json, "receiver_id");
    // Fallback to friends table naming
    if (!sender_id) sender_id = cJSON_GetObjectItem(json, "requester_id");
    if (!receiver_id) receiver_id = cJSON_GetObjectItem(json, "addressee_id");
    cJSON *status = cJSON_GetObjectItem(json, "status");

    if (id && cJSON_IsNumber(id)) req->id = id->valueint;
    if (sender_id && cJSON_IsNumber(sender_id)) req->sender_id = sender_id->valueint;
    if (receiver_id && cJSON_IsNumber(receiver_id)) req->receiver_id = receiver_id->valueint;
    if (status && cJSON_IsString(status)) req->status = strdup(status->valuestring);

    req->created_at = 0;
    req->updated_at = 0;

    return req;
}

static friend_t* parse_friend_from_json(cJSON *json) {
    if (!json) return NULL;

    friend_t *f = calloc(1, sizeof(friend_t));
    if (!f) return NULL;

    cJSON *id = cJSON_GetObjectItem(json, "id");
    cJSON *requester_id = cJSON_GetObjectItem(json, "requester_id");
    cJSON *addressee_id = cJSON_GetObjectItem(json, "addressee_id");
    cJSON *status = cJSON_GetObjectItem(json, "status");

    if (id && cJSON_IsNumber(id)) f->id = id->valueint;
    if (requester_id && cJSON_IsNumber(requester_id)) f->requester_id = requester_id->valueint;
    if (addressee_id && cJSON_IsNumber(addressee_id)) f->addressee_id = addressee_id->valueint;
    if (status && cJSON_IsString(status)) f->status = strdup(status->valuestring);

    f->created_at = 0;
    f->updated_at = 0;

    return f;
}

// ============================================================================
// FRIEND REQUESTS
// ============================================================================

db_error_t friend_request_create(
    int32_t sender_id,
    int32_t receiver_id,
    friend_request_t **out_request
) {
    printf("[FRIEND] Creating friend request: sender=%d, receiver=%d\n", sender_id, receiver_id);
    
    if (sender_id <= 0 || receiver_id <= 0 || !out_request) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Check self-request
    if (sender_id == receiver_id) {
        printf("[FRIEND] Error: User cannot add themselves\n");
        return DB_ERROR_INVALID_PARAM;
    }

    // Check if already friends (either direction is ACCEPTED)
    char check_friends_query[512];
    snprintf(check_friends_query, sizeof(check_friends_query),
        "SELECT id FROM friends WHERE "
        "((requester_id = %d AND addressee_id = %d) OR (requester_id = %d AND addressee_id = %d)) "
        "AND status = 'ACCEPTED' LIMIT 1",
        sender_id, receiver_id, receiver_id, sender_id);

    cJSON *friends_response = NULL;
    db_error_t check_err = db_get(NULL, check_friends_query, &friends_response);
    if (check_err == DB_SUCCESS && cJSON_IsArray(friends_response) && cJSON_GetArraySize(friends_response) > 0) {
        cJSON_Delete(friends_response);
        printf("[FRIEND] Error: Already friends with user %d\n", receiver_id);
        return DB_ERR_CONFLICT;
    }
    if (friends_response) cJSON_Delete(friends_response);

    // Check if pending request already exists FROM sender TO receiver
    char check_pending_query[256];
    snprintf(check_pending_query, sizeof(check_pending_query),
        "SELECT id FROM friends WHERE requester_id = %d AND addressee_id = %d AND status = 'PENDING' LIMIT 1",
        sender_id, receiver_id);

    cJSON *pending_response = NULL;
    check_err = db_get(NULL, check_pending_query, &pending_response);
    if (check_err == DB_SUCCESS && cJSON_IsArray(pending_response) && cJSON_GetArraySize(pending_response) > 0) {
        cJSON_Delete(pending_response);
        printf("[FRIEND] Error: Pending request already exists from %d to %d\n", sender_id, receiver_id);
        return DB_ERR_CONFLICT;
    }
    if (pending_response) cJSON_Delete(pending_response);

    // Check if receiver already sent a pending request TO sender (mutual request case)
    char check_mutual_query[256];
    snprintf(check_mutual_query, sizeof(check_mutual_query),
        "SELECT id FROM friends WHERE requester_id = %d AND addressee_id = %d AND status = 'PENDING' LIMIT 1",
        receiver_id, sender_id);

    cJSON *mutual_response = NULL;
    check_err = db_get(NULL, check_mutual_query, &mutual_response);
    if (check_err == DB_SUCCESS && cJSON_IsArray(mutual_response) && cJSON_GetArraySize(mutual_response) > 0) {
        cJSON_Delete(mutual_response);
        printf("[FRIEND] Error: Receiver has already sent pending request to sender\n");
        return DB_ERR_CONFLICT;
    }
    if (mutual_response) cJSON_Delete(mutual_response);

    // All checks passed, create the request
    char query[256];
    snprintf(query, sizeof(query),
        "INSERT INTO friends (requester_id, addressee_id, status) VALUES (%d, %d, 'PENDING') RETURNING id, requester_id AS sender_id, addressee_id AS receiver_id, status, created_at, updated_at",
        sender_id, receiver_id);

    cJSON *response = NULL;
    db_error_t err = db_get(NULL, query, &response);

    if (err != DB_SUCCESS) {
        printf("[FRIEND] Error creating friend request: %d\n", err);
        if (response) cJSON_Delete(response);
        return err;
    }

    if (cJSON_IsArray(response) && cJSON_GetArraySize(response) > 0) {
        cJSON *item = cJSON_GetArrayItem(response, 0);
        *out_request = parse_friend_request_from_json(item);
        cJSON_Delete(response);
        return *out_request ? DB_SUCCESS : DB_ERROR_PARSE;
    }

    cJSON_Delete(response);
    return DB_ERROR_PARSE;
}

db_error_t friend_request_list(
    int32_t receiver_id,
    friend_request_t ***out_requests,
    int32_t *out_count
) {
    printf("[FRIEND] Loading friend requests for receiver=%d\n", receiver_id);
    
    if (receiver_id <= 0 || !out_requests || !out_count) {
        return DB_ERROR_INVALID_PARAM;
    }

    char query[256];
    snprintf(query, sizeof(query),
        "SELECT id, requester_id AS sender_id, addressee_id AS receiver_id, status, created_at, updated_at FROM friends WHERE addressee_id = %d AND status = 'PENDING' ORDER BY created_at DESC",
        receiver_id);

    cJSON *response = NULL;
    db_error_t err = db_get(NULL, query, &response);

    if (err != DB_SUCCESS) {
        printf("[FRIEND] Error fetching requests: %d\n", err);
        if (response) cJSON_Delete(response);
        return err;
    }

    if (!cJSON_IsArray(response)) {
        printf("[FRIEND] Response is not array\n");
        cJSON_Delete(response);
        return DB_ERROR_PARSE;
    }

    int32_t count = cJSON_GetArraySize(response);
    printf("[FRIEND] Found %d pending requests\n", count);

    if (count == 0) {
        *out_requests = NULL;
        *out_count = 0;
        cJSON_Delete(response);
        return DB_SUCCESS;
    }

    friend_request_t **reqs = calloc(count, sizeof(friend_request_t*));
    if (!reqs) {
        cJSON_Delete(response);
        return DB_ERROR_INTERNAL;
    }

    for (int32_t i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(response, i);
        reqs[i] = parse_friend_request_from_json(item);
    }

    cJSON_Delete(response);
    *out_requests = reqs;
    *out_count = count;
    return DB_SUCCESS;
}

db_error_t friend_request_accept(
    int32_t request_id,
    friend_request_t **out_request
) {
    printf("[FRIEND] Accepting friend request id=%d\n", request_id);
    
    if (request_id <= 0 || !out_request) {
        return DB_ERROR_INVALID_PARAM;
    }

    // First, get the original request to know requester and addressee
    char get_query[256];
    snprintf(get_query, sizeof(get_query),
        "SELECT id, requester_id, addressee_id, status FROM friends WHERE id=%d",
        request_id);

    cJSON *get_response = NULL;
    db_error_t err = db_get(NULL, get_query, &get_response);
    if (err != DB_SUCCESS || !cJSON_IsArray(get_response) || cJSON_GetArraySize(get_response) == 0) {
        if (get_response) cJSON_Delete(get_response);
        return DB_ERROR_PARSE;
    }

    cJSON *request_item = cJSON_GetArrayItem(get_response, 0);
    int32_t requester_id = cJSON_GetObjectItem(request_item, "requester_id")->valueint;
    int32_t addressee_id = cJSON_GetObjectItem(request_item, "addressee_id")->valueint;

    // Update the original request to ACCEPTED
    char update_query[256];
    snprintf(update_query, sizeof(update_query),
        "UPDATE friends SET status='ACCEPTED', updated_at=NOW() WHERE id=%d RETURNING id, requester_id AS sender_id, addressee_id AS receiver_id, status, created_at, updated_at",
        request_id);

    cJSON *update_response = NULL;
    err = db_get(NULL, update_query, &update_response);
    cJSON_Delete(get_response);

    if (err != DB_SUCCESS) {
        printf("[FRIEND] Error updating request: %d\n", err);
        if (update_response) cJSON_Delete(update_response);
        return err;
    }

    // Insert reverse friendship (addressee -> requester) as ACCEPTED
    char insert_query[512];
    snprintf(insert_query, sizeof(insert_query),
        "INSERT INTO friends (requester_id, addressee_id, status) VALUES (%d, %d, 'ACCEPTED') ON CONFLICT (requester_id, addressee_id) DO UPDATE SET status='ACCEPTED', updated_at=NOW() RETURNING id",
        addressee_id, requester_id);

    cJSON *insert_response = NULL;
    err = db_get(NULL, insert_query, &insert_response);
    if (insert_response) cJSON_Delete(insert_response);

    if (cJSON_IsArray(update_response) && cJSON_GetArraySize(update_response) > 0) {
        cJSON *item = cJSON_GetArrayItem(update_response, 0);
        *out_request = parse_friend_request_from_json(item);
        cJSON_Delete(update_response);
        return *out_request ? DB_SUCCESS : DB_ERROR_PARSE;
    }

    cJSON_Delete(update_response);
    return DB_ERROR_PARSE;
}

db_error_t friend_request_reject(
    int32_t request_id,
    friend_request_t **out_request
) {
    printf("[FRIEND] Rejecting friend request id=%d\n", request_id);
    
    if (request_id <= 0 || !out_request) {
        return DB_ERROR_INVALID_PARAM;
    }

    char query[256];
    snprintf(query, sizeof(query),
        "UPDATE friends SET status='REJECTED', updated_at=NOW() WHERE id=%d RETURNING id, requester_id AS sender_id, addressee_id AS receiver_id, status, created_at, updated_at",
        request_id);

    cJSON *response = NULL;
    db_error_t err = db_get(NULL, query, &response);

    if (err != DB_SUCCESS) {
        if (response) cJSON_Delete(response);
        return err;
    }

    if (cJSON_IsArray(response) && cJSON_GetArraySize(response) > 0) {
        cJSON *item = cJSON_GetArrayItem(response, 0);
        *out_request = parse_friend_request_from_json(item);
        cJSON_Delete(response);
        return *out_request ? DB_SUCCESS : DB_ERROR_PARSE;
    }

    cJSON_Delete(response);
    return DB_ERROR_PARSE;
}

// ============================================================================
// FRIENDSHIPS
// ============================================================================

db_error_t friend_list_get_ids(
    int32_t user_id,
    int32_t **out_friend_ids,
    int32_t *out_count
) {
    printf("[FRIEND] Loading friend IDs for user=%d\n", user_id);
    
    if (user_id <= 0 || !out_friend_ids || !out_count) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Query friendships where requester_id = user_id OR addressee_id = user_id (and status = ACCEPTED)
    // Get addressee_id when requester_id = user_id, or requester_id when addressee_id = user_id
    char query[512];
    snprintf(query, sizeof(query), 
        "SELECT addressee_id FROM friends WHERE requester_id = %d AND status = 'ACCEPTED' "
        "UNION "
        "SELECT requester_id FROM friends WHERE addressee_id = %d AND status = 'ACCEPTED'",
        user_id, user_id);

    cJSON *response = NULL;
    db_error_t err = db_get("friends", query, &response);

    if (err != DB_SUCCESS) {
        printf("[FRIEND] Error fetching friends: %d\n", err);
        if (response) cJSON_Delete(response);
        return err;
    }

    if (!cJSON_IsArray(response)) {
        printf("[FRIEND] Response is not array\n");
        cJSON_Delete(response);
        return DB_ERROR_PARSE;
    }

    int32_t count = cJSON_GetArraySize(response);
    printf("[FRIEND] Found %d friends\n", count);

    if (count == 0) {
        *out_friend_ids = NULL;
        *out_count = 0;
        cJSON_Delete(response);
        return DB_SUCCESS;
    }

    int32_t *ids = calloc(count, sizeof(int32_t));
    if (!ids) {
        cJSON_Delete(response);
        return DB_ERROR_INTERNAL;
    }

    for (int32_t i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(response, i);
        cJSON *addressee_id = cJSON_GetObjectItem(item, "addressee_id");
        if (addressee_id && cJSON_IsNumber(addressee_id)) {
            ids[i] = addressee_id->valueint;
        }
    }

    cJSON_Delete(response);
    *out_friend_ids = ids;
    *out_count = count;
    return DB_SUCCESS;
}

db_error_t friend_list_get_full(
    int32_t user_id,
    friend_t ***out_friends,
    int32_t *out_count
) {
    printf("[FRIEND] Loading full friend list for user=%d\n", user_id);
    
    if (user_id <= 0 || !out_friends || !out_count) {
        return DB_ERROR_INVALID_PARAM;
    }

    char query[512];
    snprintf(query, sizeof(query), 
        "SELECT * FROM friends WHERE requester_id = %d AND status = 'ACCEPTED' "
        "UNION ALL "
        "SELECT * FROM friends WHERE addressee_id = %d AND status = 'ACCEPTED' "
        "ORDER BY created_at DESC",
        user_id, user_id);

    cJSON *response = NULL;
    db_error_t err = db_get("friends", query, &response);

    if (err != DB_SUCCESS) {
        if (response) cJSON_Delete(response);
        return err;
    }

    if (!cJSON_IsArray(response)) {
        cJSON_Delete(response);
        return DB_ERROR_PARSE;
    }

    int32_t count = cJSON_GetArraySize(response);

    if (count == 0) {
        *out_friends = NULL;
        *out_count = 0;
        cJSON_Delete(response);
        return DB_SUCCESS;
    }

    friend_t **friends = calloc(count, sizeof(friend_t*));
    if (!friends) {
        cJSON_Delete(response);
        return DB_ERROR_INTERNAL;
    }

    for (int32_t i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(response, i);
        friends[i] = parse_friend_from_json(item);
    }

    cJSON_Delete(response);
    *out_friends = friends;
    *out_count = count;
    return DB_SUCCESS;
}

db_error_t friend_check_relationship(
    int32_t user_id_1,
    int32_t user_id_2,
    bool *out_are_friends
) {
    if (user_id_1 <= 0 || user_id_2 <= 0 || !out_are_friends) {
        return DB_ERROR_INVALID_PARAM;
    }

    char query[256];
    snprintf(query, sizeof(query), 
        "SELECT id FROM friends WHERE requester_id = %d AND addressee_id = %d AND status = 'accepted' LIMIT 1",
        user_id_1, user_id_2);

    cJSON *response = NULL;
    db_error_t err = db_get("friends", query, &response);

    if (err != DB_SUCCESS) {
        *out_are_friends = false;
        if (response) cJSON_Delete(response);
        return DB_SUCCESS; // Treat as "not friends" rather than error
    }

    bool is_friends = cJSON_IsArray(response) && cJSON_GetArraySize(response) > 0;
    cJSON_Delete(response);

    *out_are_friends = is_friends;
    return DB_SUCCESS;
}

db_error_t friend_remove(
    int32_t user_id_1,
    int32_t user_id_2
) {
    printf("[FRIEND] Removing friendship: %d <-> %d\n", user_id_1, user_id_2);
    
    if (user_id_1 <= 0 || user_id_2 <= 0) {
        return DB_ERROR_INVALID_PARAM;
    }

    // Remove both directions
    char filter1[256];
    snprintf(filter1, sizeof(filter1), 
        "and(requester_id.eq.%d,addressee_id.eq.%d)", user_id_1, user_id_2);

    char filter2[256];
    snprintf(filter2, sizeof(filter2), 
        "and(requester_id.eq.%d,addressee_id.eq.%d)", user_id_2, user_id_1);

    db_error_t err1 = db_delete("friends", filter1, NULL);
    db_error_t err2 = db_delete("friends", filter2, NULL);

    // Return success if at least one delete succeeded or both returned not found
    if (err1 == DB_SUCCESS || err2 == DB_SUCCESS) {
        return DB_SUCCESS;
    }

    return err1; // Return first error if both failed
}

db_error_t friend_block(
    int32_t user_id,
    int32_t blocked_user_id
) {
    // TODO: Implement blocking feature
    (void)user_id;
    (void)blocked_user_id;
    return DB_ERROR_NOT_IMPLEMENTED;
}

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

void friend_request_free(friend_request_t *req) {
    if (!req) return;
    if (req->status) free(req->status);
    free(req);
}

void friend_request_free_array(friend_request_t **reqs, int32_t count) {
    if (!reqs) return;
    for (int32_t i = 0; i < count; i++) {
        friend_request_free(reqs[i]);
    }
    free(reqs);
}

void friend_free(friend_t *f) {
    if (!f) return;
    if (f->status) free(f->status);
    free(f);
}

void friend_free_array(friend_t **friends, int32_t count) {
    if (!friends) return;
    for (int32_t i = 0; i < count; i++) {
        friend_free(friends[i]);
    }
    free(friends);
}
