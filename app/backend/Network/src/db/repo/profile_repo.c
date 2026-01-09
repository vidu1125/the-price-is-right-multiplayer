#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "db/repo/profile_repo.h"
#include "db/core/db_client.h"

static profile_t* parse_profile_from_json(cJSON *json) {
	if (!json) return NULL;

	profile_t *profile = calloc(1, sizeof(profile_t));
	if (!profile) return NULL;

	cJSON *id = cJSON_GetObjectItem(json, "id");
	cJSON *account_id = cJSON_GetObjectItem(json, "account_id");
	cJSON *name = cJSON_GetObjectItem(json, "name");
	cJSON *avatar = cJSON_GetObjectItem(json, "avatar");
	cJSON *bio = cJSON_GetObjectItem(json, "bio");
	cJSON *matches = cJSON_GetObjectItem(json, "matches");
	cJSON *wins = cJSON_GetObjectItem(json, "wins");
	cJSON *points = cJSON_GetObjectItem(json, "points");
	cJSON *badges = cJSON_GetObjectItem(json, "badges");
	(void)cJSON_GetObjectItem(json, "created_at");
	(void)cJSON_GetObjectItem(json, "updated_at");

	if (id && cJSON_IsNumber(id)) profile->id = id->valueint;
	if (account_id && cJSON_IsNumber(account_id)) profile->account_id = account_id->valueint;
	if (name && cJSON_IsString(name)) profile->name = strdup(name->valuestring);
	if (avatar && cJSON_IsString(avatar)) profile->avatar = strdup(avatar->valuestring);
	if (bio && cJSON_IsString(bio)) profile->bio = strdup(bio->valuestring);
	if (matches && cJSON_IsNumber(matches)) profile->matches = matches->valueint;
	if (wins && cJSON_IsNumber(wins)) profile->wins = wins->valueint;
	if (points && cJSON_IsNumber(points)) profile->points = points->valueint;
	if (badges && cJSON_IsString(badges)) profile->badges = strdup(badges->valuestring);

	profile->created_at = 0;
	profile->updated_at = 0;
	return profile;
}

db_error_t profile_create(
	int32_t account_id,
	const char *name,
	const char *avatar,
	const char *bio,
	profile_t **out_profile
) {
	if (account_id <= 0 || !out_profile) {
		return DB_ERROR_INVALID_PARAM;
	}

	cJSON *payload = cJSON_CreateObject();
	cJSON_AddNumberToObject(payload, "account_id", account_id);
	if (name && strlen(name) > 0) cJSON_AddStringToObject(payload, "name", name);
	if (avatar && strlen(avatar) > 0) cJSON_AddStringToObject(payload, "avatar", avatar);
	if (bio && strlen(bio) > 0) cJSON_AddStringToObject(payload, "bio", bio);
	cJSON_AddNumberToObject(payload, "matches", 0);
	cJSON_AddNumberToObject(payload, "wins", 0);
	cJSON_AddNumberToObject(payload, "points", 0);
	cJSON_AddStringToObject(payload, "badges", "[]");

	cJSON *response = NULL;
	db_error_t err = db_post("profiles", payload, &response);
	cJSON_Delete(payload);

	if (err != DB_SUCCESS) {
		if (response) cJSON_Delete(response);
		return err;
	}

	if (cJSON_IsArray(response) && cJSON_GetArraySize(response) > 0) {
		cJSON *item = cJSON_GetArrayItem(response, 0);
		*out_profile = parse_profile_from_json(item);
		cJSON_Delete(response);
		return *out_profile ? DB_SUCCESS : DB_ERROR_PARSE;
	}

	cJSON_Delete(response);
	return DB_ERROR_PARSE;
}

db_error_t profile_find_by_account(
	int32_t account_id,
	profile_t **out_profile
) {
	if (account_id <= 0 || !out_profile) {
		return DB_ERROR_INVALID_PARAM;
	}

	char query[256];
	snprintf(query, sizeof(query), "select=*&account_id=eq.%d&limit=1", account_id);

	cJSON *response = NULL;
	db_error_t err = db_get("profiles", query, &response);

	if (err != DB_SUCCESS) {
		if (response) cJSON_Delete(response);
		return err;
	}

	if (cJSON_IsArray(response)) {
		if (cJSON_GetArraySize(response) == 0) {
			cJSON_Delete(response);
			return DB_ERROR_NOT_FOUND;
		}
		cJSON *item = cJSON_GetArrayItem(response, 0);
		*out_profile = parse_profile_from_json(item);
		cJSON_Delete(response);
		return *out_profile ? DB_SUCCESS : DB_ERROR_PARSE;
	}

	cJSON_Delete(response);
	return DB_ERROR_PARSE;
}

void profile_free(profile_t *profile) {
	if (!profile) return;
	free(profile->name);
	free(profile->avatar);
	free(profile->bio);
	free(profile->badges);
	free(profile);
}
