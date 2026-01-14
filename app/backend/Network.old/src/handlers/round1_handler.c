#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <time.h>

#include "handlers/round1_handler.h"
#include "protocol/opcode.h"
#include "protocol/protocol.h"
#include "db/core/db_client.h"
#include <cjson/cJSON.h>

// Payload structures
#if defined(__GNUC__) || defined(__clang__)
  #define PACKED __attribute__((packed))
#else
  #pragma pack(push, 1)
  #define PACKED
#endif

typedef struct PACKED {
  uint32_t room_id;
  uint32_t match_id;
} Round1ReadyPayload;

typedef struct PACKED {
  uint32_t match_id;
  uint32_t player_id;
  uint32_t question_idx;
} GetQuestionPayload;

typedef struct PACKED {
  uint32_t match_id;
  uint32_t player_id;
  uint32_t question_idx;
  uint8_t  choice;
  uint32_t time_taken_ms;
} SubmitAnswerPayload;

typedef struct PACKED {
  uint32_t match_id;
  uint32_t player_id;
} EndRoundPayload;

#if !defined(__GNUC__) && !defined(__clang__)
  #pragma pack(pop)
#endif

//==============================================================================
// RANDOM QUESTION INDICES - Pre-shuffle for each match
//==============================================================================
#define MAX_QUESTIONS 100
#define QUESTIONS_PER_ROUND 10
#define MAX_PLAYERS 4

static int g_random_indices[QUESTIONS_PER_ROUND];
static int g_total_questions = 0;
static bool g_indices_initialized = false;

//==============================================================================
// PLAYER STATE TRACKING
//==============================================================================
typedef struct {
  int client_fd;
  uint32_t player_id;
  bool is_ready;
  bool is_finished;
  int score;
} PlayerState;

static PlayerState g_players[MAX_PLAYERS];
static int g_player_count = 0;
static int g_ready_count = 0;
static int g_finished_count = 0;
static bool g_game_started = false;
static uint32_t g_current_match_id = 0;

static void reset_player_states(void) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    g_players[i].client_fd = -1;
    g_players[i].player_id = 0;
    g_players[i].is_ready = false;
    g_players[i].is_finished = false;
    g_players[i].score = 0;
  }
  g_player_count = 0;
  g_ready_count = 0;
  g_finished_count = 0;
  g_game_started = false;
}

static int find_player_index(int client_fd) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_players[i].client_fd == client_fd) return i;
  }
  return -1;
}

static int add_or_get_player(int client_fd, uint32_t player_id) {
  printf("[Round1] add_or_get_player: client_fd=%d, player_id=%u\n", client_fd, player_id);
  
  // First check if this player_id already exists (including disconnected players)
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_players[i].player_id == player_id) {
      bool was_disconnected = (g_players[i].client_fd == -1);
      printf("[Round1] Player %u found at slot %d (was_disconnected=%d), updating fd from %d to %d\n", 
             player_id, i, was_disconnected, g_players[i].client_fd, client_fd);
      g_players[i].client_fd = client_fd; // Update to new fd
      // Don't change other state - preserve score, is_finished, etc.
      return i;
    }
  }
  
  // Find empty slot for new player (player_id == 0 means unused slot)
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_players[i].player_id == 0) {
      g_players[i].client_fd = client_fd;
      g_players[i].player_id = player_id;
      g_players[i].is_ready = false;
      g_players[i].is_finished = false;
      g_players[i].score = 0;
      g_player_count++;
      printf("[Round1] Added NEW player %u at slot %d (fd=%d), total=%d\n", 
             player_id, i, client_fd, g_player_count);
      return i;
    }
  }
  printf("[Round1] No empty slot! All %d slots used\n", MAX_PLAYERS);
  return -1; // Full
}

static void shuffle_question_indices(int total, int count) {
  if (total <= 0) return;
  if (count > total) count = total;
  
  int *all = malloc(total * sizeof(int));
  if (!all) return;
  
  for (int i = 0; i < total; i++) all[i] = i;
  
  srand((unsigned int)time(NULL));
  for (int i = 0; i < count && i < total; i++) {
    int j = i + rand() % (total - i);
    int tmp = all[i];
    all[i] = all[j];
    all[j] = tmp;
    g_random_indices[i] = all[i];
  }
  
  free(all);
  
  printf("[Round1] Shuffled %d random questions from %d total: ", count, total);
  for (int i = 0; i < count; i++) printf("%d ", g_random_indices[i]);
  printf("\n");
}

static int get_total_question_count(void) {
  // Chỉ đếm câu hỏi mcq và active=true theo schema mới (type là lowercase!)
  cJSON *arr = NULL;
  db_error_t rc = db_get("questions", "select=id&active=eq.true&type=eq.mcq", &arr);
  if (rc != DB_OK || !arr || !cJSON_IsArray(arr)) {
    if (arr) cJSON_Delete(arr);
    printf("[Round1] Failed to get question count, rc=%d\n", rc);
    return 0;
  }
  int count = cJSON_GetArraySize(arr);
  cJSON_Delete(arr);
  printf("[Round1] Total mcq questions in DB: %d\n", count);
  return count;
}

static void ensure_random_indices(void) {
  if (!g_indices_initialized) {
    g_total_questions = get_total_question_count();
    if (g_total_questions > 0) {
      shuffle_question_indices(g_total_questions, QUESTIONS_PER_ROUND);
      g_indices_initialized = true;
    } else {
      printf("[Round1] WARNING: No questions found in database!\n");
    }
  }
}

static void reset_random_indices(void) {
  g_indices_initialized = false;
}

//==============================================================================
// HELPER: Send JSON
//==============================================================================
static void send_json_response(
  int client_fd,
  MessageHeader *req,
  uint16_t cmd,
  const char *json
) {
  forward_response(client_fd, req, cmd, json, (uint32_t)strlen(json));
}

//==============================================================================
// HELPER: Broadcast to all players
//==============================================================================
static void broadcast_to_all_players(MessageHeader *req, uint16_t cmd, const char *json) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_players[i].client_fd >= 0) {
      printf("[Round1] Broadcasting 0x%04X to player %d (fd=%d)\n", 
             cmd, g_players[i].player_id, g_players[i].client_fd);
      forward_response(g_players[i].client_fd, req, cmd, json, (uint32_t)strlen(json));
    }
  }
}

//==============================================================================
// HELPER: Build ready status JSON
//==============================================================================
static char* build_ready_status_json(void) {
  cJSON *resp = cJSON_CreateObject();
  cJSON_AddTrueToObject(resp, "success");
  cJSON_AddNumberToObject(resp, "ready_count", g_ready_count);
  cJSON_AddNumberToObject(resp, "player_count", g_player_count);
  cJSON_AddNumberToObject(resp, "required_players", MAX_PLAYERS);
  
  cJSON *players = cJSON_CreateArray();
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_players[i].client_fd >= 0) {
      cJSON *p = cJSON_CreateObject();
      cJSON_AddNumberToObject(p, "id", g_players[i].player_id);
      char name[32];
      snprintf(name, sizeof(name), "PLAYER%u", g_players[i].player_id);
      cJSON_AddStringToObject(p, "name", name);
      cJSON_AddBoolToObject(p, "is_ready", g_players[i].is_ready);
      cJSON_AddItemToArray(players, p);
    }
  }
  cJSON_AddItemToObject(resp, "players", players);
  
  char *json = cJSON_PrintUnformatted(resp);
  cJSON_Delete(resp);
  return json;
}

//==============================================================================
// HELPER: Build final scoreboard JSON
//==============================================================================
static char* build_scoreboard_json(void) {
  cJSON *resp = cJSON_CreateObject();
  cJSON_AddTrueToObject(resp, "success");
  cJSON_AddNumberToObject(resp, "match_id", g_current_match_id);
  cJSON_AddNumberToObject(resp, "round_completed", 1);
  
  cJSON *players = cJSON_CreateArray();
  int connected_count = 0;
  
  for (int i = 0; i < MAX_PLAYERS; i++) {
    // Include all players who have a player_id (even if disconnected)
    if (g_players[i].player_id > 0) {
      cJSON *p = cJSON_CreateObject();
      cJSON_AddNumberToObject(p, "id", g_players[i].player_id);
      char name[32];
      snprintf(name, sizeof(name), "PLAYER%u", g_players[i].player_id);
      cJSON_AddStringToObject(p, "name", name);
      cJSON_AddNumberToObject(p, "score", g_players[i].score);
      
      // Add connected status
      bool is_connected = (g_players[i].client_fd >= 0);
      cJSON_AddBoolToObject(p, "connected", is_connected);
      if (is_connected) connected_count++;
      
      cJSON_AddItemToArray(players, p);
    }
  }
  cJSON_AddItemToObject(resp, "players", players);
  cJSON_AddNumberToObject(resp, "connected_count", connected_count);
  
  char *json = cJSON_PrintUnformatted(resp);
  cJSON_Delete(resp);
  return json;
}

//==============================================================================
// HELPER: Read question by offset
//==============================================================================
static int get_question_by_offset(int offset, char *out_buf, size_t out_size, int *out_correct_index) {
  // Query với schema mới: select type và data (JSONB), chỉ lấy mcq questions (type là lowercase!)
  char query[256];
  snprintf(query, sizeof(query), "select=id,type,data&active=eq.true&type=eq.mcq&order=id.asc&limit=1&offset=%d", offset);

  printf("[Round1] Fetching question with query: %s\n", query);

  cJSON *arr = NULL;
  db_error_t rc = db_get("questions", query, &arr);
  if (rc != DB_OK || !arr || !cJSON_IsArray(arr) || cJSON_GetArraySize(arr) == 0) {
    printf("[Round1] DB get questions failed rc=%d offset=%d\n", rc, offset);
    if (arr) cJSON_Delete(arr);
    return -1;
  }

  cJSON *row = cJSON_GetArrayItem(arr, 0);

  // Lấy type và data từ schema mới
  cJSON *type_obj = cJSON_GetObjectItem(row, "type");
  cJSON *data_obj = cJSON_GetObjectItem(row, "data");

  if (!type_obj || !cJSON_IsString(type_obj) || !data_obj) {
    printf("[Round1] Missing type or data fields\n");
    cJSON_Delete(arr);
    return -1;
  }

  const char *type = type_obj->valuestring;
  
  // Parse data field (có thể là string JSON hoặc đã là object)
  cJSON *data = NULL;
  if (cJSON_IsString(data_obj)) {
    // Nếu data là string, parse nó thành JSON object
    data = cJSON_Parse(data_obj->valuestring);
    if (!data) {
      printf("[Round1] Failed to parse data JSON string\n");
      cJSON_Delete(arr);
      return -1;
    }
  } else if (cJSON_IsObject(data_obj)) {
    // Nếu data đã là object, duplicate để tránh bị delete khi delete arr
    data = cJSON_Duplicate(data_obj, 1);
  } else {
    printf("[Round1] Data field is not string or object\n");
    cJSON_Delete(arr);
    return -1;
  }

  // Xử lý theo type (hỗ trợ cả lowercase và uppercase)
  if (strcmp(type, "mcq") == 0 || strcmp(type, "MCQ") == 0) {
    // MCQ format: { "question": "...", "choices": [...], "image": "..." }
    cJSON *question_text = cJSON_GetObjectItem(data, "question");
    cJSON *choices_array = cJSON_GetObjectItem(data, "choices");
    cJSON *correct_answer = cJSON_GetObjectItem(data, "correct_answer");
    cJSON *correct_index_obj = cJSON_GetObjectItem(data, "correct_index");

    if (!question_text || !cJSON_IsString(question_text) || !choices_array || !cJSON_IsArray(choices_array)) {
      printf("[Round1] Invalid MCQ data structure\n");
      cJSON_Delete(data);
      cJSON_Delete(arr);
      return -1;
    }

    int choice_count = cJSON_GetArraySize(choices_array);
    if (choice_count < 1 || choice_count > 4) {
      printf("[Round1] Invalid choices count: %d\n", choice_count);
      cJSON_Delete(data);
      cJSON_Delete(arr);
      return -1;
    }

    // Xác định correct_index
    // Có thể có correct_index (số), correct_answer (chuỗi), hoặc correct (ký tự a/b/c/d)
    int correct_index = -1;
    if (correct_index_obj && cJSON_IsNumber(correct_index_obj)) {
      correct_index = correct_index_obj->valueint;
      printf("[Round1] Using correct_index from data: %d\n", correct_index);
    } else if (correct_answer) {
      if (cJSON_IsString(correct_answer)) {
        // Tìm index của correct_answer trong choices
        const char *correct_str = correct_answer->valuestring;
        printf("[Round1] Looking for correct_answer: %s\n", correct_str);
        for (int i = 0; i < choice_count; i++) {
          cJSON *choice = cJSON_GetArrayItem(choices_array, i);
          if (choice && cJSON_IsString(choice) && strcmp(choice->valuestring, correct_str) == 0) {
            correct_index = i;
            printf("[Round1] Found correct_answer at index %d\n", i);
            break;
          }
        }
      } else if (cJSON_IsNumber(correct_answer)) {
        // Nếu correct_answer là số, có thể là index trực tiếp
        correct_index = correct_answer->valueint;
        printf("[Round1] Using correct_answer as index: %d\n", correct_index);
      }
    } else {
      // Thử tìm field "correct" (ký tự a/b/c/d như schema cũ)
      cJSON *correct_char = cJSON_GetObjectItem(data, "correct");
      if (correct_char && cJSON_IsString(correct_char)) {
        const char *corr = correct_char->valuestring;
        if (corr && corr[0]) {
          switch (corr[0]) {
            case 'a': case 'A': correct_index = 0; break;
            case 'b': case 'B': correct_index = 1; break;
            case 'c': case 'C': correct_index = 2; break;
            case 'd': case 'D': correct_index = 3; break;
          }
          printf("[Round1] Using correct char '%c' -> index %d\n", corr[0], correct_index);
        }
      }
    }

    // Nếu không tìm thấy correct_index, default là 0 (đáp án đầu tiên)
    // WARNING: Dữ liệu trong DB đang thiếu trường correct_answer/correct_index!
    if (correct_index < 0 || correct_index >= choice_count) {
      printf("[Round1] WARNING: No correct_index found in data, defaulting to 0. Please add correct_index to question data!\n");
      correct_index = 0;  // Default to first choice
    }

    // Build response JSON
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddTrueToObject(resp, "success");
    cJSON_AddStringToObject(resp, "question", question_text->valuestring);
    
    // Lấy image (có thể là "image" hoặc "product_image")
    cJSON *image_obj = cJSON_GetObjectItem(data, "image");
    if (!image_obj) {
      image_obj = cJSON_GetObjectItem(data, "product_image");
    }
    if (image_obj && cJSON_IsString(image_obj)) {
      cJSON_AddStringToObject(resp, "product_image", image_obj->valuestring);
    } else {
      cJSON_AddStringToObject(resp, "product_image", "");
    }

    // Copy choices array
    cJSON *choices = cJSON_CreateArray();
    for (int i = 0; i < choice_count; i++) {
      cJSON *choice = cJSON_GetArrayItem(choices_array, i);
      if (choice && cJSON_IsString(choice)) {
        cJSON_AddItemToArray(choices, cJSON_CreateString(choice->valuestring));
      } else {
        cJSON_AddItemToArray(choices, cJSON_CreateString(""));
      }
    }
    // Đảm bảo có đủ 4 choices
    while (cJSON_GetArraySize(choices) < 4) {
      cJSON_AddItemToArray(choices, cJSON_CreateString(""));
    }
    cJSON_AddItemToObject(resp, "choices", choices);

    cJSON_AddNumberToObject(resp, "correct_index", correct_index);

    char *s = cJSON_PrintUnformatted(resp);
    if (!s) {
      cJSON_Delete(resp);
      cJSON_Delete(data);
      cJSON_Delete(arr);
      return -1;
    }

    strncpy(out_buf, s, out_size - 1);
    out_buf[out_size - 1] = '\0';

    free(s);
    cJSON_Delete(resp);
    cJSON_Delete(data);
    cJSON_Delete(arr);

    if (out_correct_index) *out_correct_index = correct_index;
    return 0;
  } else {
    printf("[Round1] Unsupported question type: %s (only MCQ supported for Round1)\n", type);
    cJSON_Delete(data);
    cJSON_Delete(arr);
    return -1;
  }
}

//==============================================================================
// CASE 1: READY (Legacy - starts game immediately for single player testing)
//==============================================================================
static void handle_ready(int client_fd, MessageHeader *req, const char *payload) {
  if (req->length != sizeof(Round1ReadyPayload)) {
    send_json_response(client_fd, req, ERR_BAD_REQUEST,
      "{\"success\":false,\"error\":\"Invalid payload size\"}");
    return;
  }

  Round1ReadyPayload data;
  memcpy(&data, payload, sizeof(data));

  uint32_t room_id = ntohl(data.room_id);
  uint32_t match_id = ntohl(data.match_id);

  printf("[Round1] Player ready - room_id=%u match_id=%u\n", room_id, match_id);

  // Reset và shuffle lại câu hỏi cho match mới
  reset_random_indices();
  ensure_random_indices();

  char json[256];
  snprintf(json, sizeof(json),
    "{\"success\":true,\"match_id\":%u,\"room_id\":%u,\"total_questions\":%d,\"time_per_question\":15,\"round\":1}",
    match_id, room_id, QUESTIONS_PER_ROUND
  );

  send_json_response(client_fd, req, OP_S2C_ROUND1_START, json);
}

//==============================================================================
// CASE 1B: PLAYER READY (New multiplayer ready system) - with RECONNECTION support
//==============================================================================
static void handle_player_ready(int client_fd, MessageHeader *req, const char *payload) {
  // Payload: match_id(4) + player_id(4)
  if (req->length < 8) {
    send_json_response(client_fd, req, ERR_BAD_REQUEST,
      "{\"success\":false,\"error\":\"Invalid payload\"}");
    return;
  }

  uint32_t match_id = 0, player_id = 0;
  memcpy(&match_id, payload, 4);
  memcpy(&player_id, payload + 4, 4);
  match_id = ntohl(match_id);
  player_id = ntohl(player_id);

  printf("[Round1] Player %u ready for match %u\n", player_id, match_id);

  // If new match, reset everything
  if (match_id != g_current_match_id) {
    printf("[Round1] New match %u, resetting state\n", match_id);
    reset_player_states();
    reset_random_indices();
    g_current_match_id = match_id;
  }

  // Check if this player already exists (RECONNECTION case)
  bool is_reconnection = false;
  int existing_idx = -1;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_players[i].player_id == player_id) {
      existing_idx = i;
      // If client_fd is different or was -1 (disconnected), this is a reconnection
      if (g_players[i].client_fd != client_fd) {
        is_reconnection = true;
        printf("[Round1] 🔄 RECONNECTION detected for player %u (old_fd=%d, new_fd=%d)\n", 
               player_id, g_players[i].client_fd, client_fd);
      }
      break;
    }
  }

  // Add player or update their fd
  int idx = add_or_get_player(client_fd, player_id);
  if (idx < 0) {
    send_json_response(client_fd, req, ERR_ROOM_FULL,
      "{\"success\":false,\"error\":\"Game is full\"}");
    return;
  }

  // Handle RECONNECTION during active game
  if (is_reconnection && g_game_started) {
    printf("[Round1] 🔄 Player %u reconnected during active game!\n", player_id);
    
    // Update their fd (already done by add_or_get_player)
    // Increment player count since they were decremented on disconnect
    g_player_count++;
    printf("[Round1] Player count after reconnect: %d, finished: %d\n", g_player_count, g_finished_count);
    
    // Check game state and send appropriate response
    if (g_players[idx].is_finished) {
      // Player already finished round 1 - send waiting status
      printf("[Round1] Player %u already finished, sending WAITING status\n", player_id);
      char waiting_json[512];
      snprintf(waiting_json, sizeof(waiting_json),
        "{\"success\":true,\"reconnected\":true,\"finished_count\":%d,\"player_count\":%d,\"waiting\":true,\"your_score\":%d,\"message\":\"Welcome back! Waiting for others to finish Round 1.\"}",
        g_finished_count, g_player_count, g_players[idx].score);
      send_json_response(client_fd, req, OP_S2C_ROUND1_WAITING, waiting_json);
      
      // Check if everyone is now finished (reconnected player was the last)
      if (g_finished_count >= g_player_count && g_player_count > 0) {
        printf("[Round1] All players finished after reconnect! Sending scoreboard...\n");
        char *scoreboard_json = build_scoreboard_json();
        broadcast_to_all_players(req, OP_S2C_ROUND1_ALL_FINISHED, scoreboard_json);
        free(scoreboard_json);
      }
    } else {
      // Player hasn't finished - they need to continue/restart Round 1
      // For simplicity, mark them as finished with their current score
      printf("[Round1] Player %u didn't finish - marking as finished with score %d\n", 
             player_id, g_players[idx].score);
      g_players[idx].is_finished = true;
      g_finished_count++;
      
      char waiting_json[512];
      snprintf(waiting_json, sizeof(waiting_json),
        "{\"success\":true,\"reconnected\":true,\"finished_count\":%d,\"player_count\":%d,\"waiting\":true,\"your_score\":%d,\"message\":\"Welcome back! Your progress was saved. Waiting for others.\"}",
        g_finished_count, g_player_count, g_players[idx].score);
      send_json_response(client_fd, req, OP_S2C_ROUND1_WAITING, waiting_json);
      
      // Broadcast updated waiting status to others
      char broadcast_json[256];
      snprintf(broadcast_json, sizeof(broadcast_json),
        "{\"finished_count\":%d,\"player_count\":%d,\"player_rejoined\":%u}",
        g_finished_count, g_player_count, player_id);
      broadcast_to_all_players(req, OP_S2C_ROUND1_WAITING, broadcast_json);
      
      // Check if everyone is now finished
      if (g_finished_count >= g_player_count && g_player_count > 0) {
        printf("[Round1] All players finished after reconnect! Sending scoreboard...\n");
        char *scoreboard_json = build_scoreboard_json();
        broadcast_to_all_players(req, OP_S2C_ROUND1_ALL_FINISHED, scoreboard_json);
        free(scoreboard_json);
      }
    }
    
    // Notify others that this player reconnected
    char reconnect_json[256];
    snprintf(reconnect_json, sizeof(reconnect_json),
      "{\"player_id\":%u,\"name\":\"Player%u\",\"event\":\"reconnected\"}",
      player_id, player_id);
    
    MessageHeader ntf_header;
    memset(&ntf_header, 0, sizeof(ntf_header));
    ntf_header.magic = htons(MAGIC_NUMBER);
    ntf_header.version = PROTOCOL_VERSION;
    ntf_header.command = htons(0x02BE); // NTF_PLAYER_RECONNECTED
    ntf_header.length = htonl(strlen(reconnect_json));
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (g_players[i].client_fd >= 0 && g_players[i].client_fd != client_fd) {
        send(g_players[i].client_fd, &ntf_header, sizeof(ntf_header), 0);
        send(g_players[i].client_fd, reconnect_json, strlen(reconnect_json), 0);
      }
    }
    
    return; // Don't continue with normal ready flow
  }

  // Mark ready (normal flow)
  if (!g_players[idx].is_ready) {
    g_players[idx].is_ready = true;
    g_ready_count++;
    printf("[Round1] Player %u is now ready (%d/%d)\n", player_id, g_ready_count, g_player_count);
  }

  // Build and broadcast ready status
  char *status_json = build_ready_status_json();
  broadcast_to_all_players(req, OP_S2C_ROUND1_READY_STATUS, status_json);
  free(status_json);

  // Check if all players ready (minimum 1 for testing, can change to MAX_PLAYERS)
  int min_players = 4; // Change to 4 for real game
  if (g_ready_count >= min_players && g_ready_count == g_player_count && !g_game_started) {
    printf("[Round1] All %d players ready! Starting game...\n", g_ready_count);
    g_game_started = true;
    
    // Shuffle questions
    ensure_random_indices();
    
    // Broadcast ALL_READY to start game
    char start_json[512];
    snprintf(start_json, sizeof(start_json),
      "{\"success\":true,\"match_id\":%u,\"total_questions\":%d,\"time_per_question\":15,\"player_count\":%d}",
      match_id, QUESTIONS_PER_ROUND, g_player_count
    );
    broadcast_to_all_players(req, OP_S2C_ROUND1_ALL_READY, start_json);
  }
}

//==============================================================================
// CASE 2: GET QUESTION
//==============================================================================
static void handle_get_question(int client_fd, MessageHeader *req, const char *payload) {
  // Accept both 8-byte (old) and 12-byte (new with player_id) payloads
  if (req->length != 8 && req->length != sizeof(GetQuestionPayload)) {
    printf("[Round1] Invalid GET_QUESTION payload size: got %u, expected 8 or %zu\n", 
           req->length, sizeof(GetQuestionPayload));
    send_json_response(client_fd, req, ERR_BAD_REQUEST,
      "{\"success\":false,\"error\":\"Invalid payload\"}");
    return;
  }

  uint32_t match_id = 0;
  uint32_t question_idx = 0;
  
  if (req->length == 8) {
    // Old format: match_id + question_idx
    memcpy(&match_id, payload, 4);
    memcpy(&question_idx, payload + 4, 4);
    match_id = ntohl(match_id);
    question_idx = ntohl(question_idx);
  } else {
    // New format with player_id
    GetQuestionPayload data;
    memcpy(&data, payload, sizeof(data));
    match_id = ntohl(data.match_id);
    question_idx = ntohl(data.question_idx);
  }

  printf("[Round1] Get question - match=%u idx=%u\n", match_id, question_idx);

  // Đảm bảo đã có random indices
  ensure_random_indices();
  
  // Lấy offset thực từ mảng random
  int actual_offset = 0;
  if (question_idx < (uint32_t)QUESTIONS_PER_ROUND && g_total_questions > 0) {
    actual_offset = g_random_indices[question_idx];
  } else if (g_total_questions > 0) {
    actual_offset = (int)question_idx % g_total_questions;
  }
  
  printf("[Round1] Using random offset=%d for idx=%u (total=%d)\n", actual_offset, question_idx, g_total_questions);

  char qbuf[4096];
  int correct_index = -1;
  
  if (get_question_by_offset(actual_offset, qbuf, sizeof(qbuf), &correct_index) != 0) {
    send_json_response(client_fd, req, ERR_SERVER_ERROR,
      "{\"success\":false,\"error\":\"Failed to load question\"}");
    return;
  }

  cJSON *obj = cJSON_Parse(qbuf);
  if (!obj) {
    send_json_response(client_fd, req, ERR_SERVER_ERROR,
      "{\"success\":false,\"error\":\"Question JSON parse failed\"}");
    return;
  }

  cJSON_AddNumberToObject(obj, "match_id", match_id);
  cJSON_AddNumberToObject(obj, "question_idx", question_idx);

  char *final_json = cJSON_PrintUnformatted(obj);
  cJSON_Delete(obj);

  if (!final_json) {
    send_json_response(client_fd, req, ERR_SERVER_ERROR,
      "{\"success\":false,\"error\":\"Failed to build response\"}");
    return;
  }

  printf("[Round1] Sending question %u: %.100s...\n", question_idx, final_json);
  send_json_response(client_fd, req, OP_S2C_ROUND1_QUESTION, final_json);
  free(final_json);
}

//==============================================================================
// CASE 3: SUBMIT ANSWER - Fixed to handle 17-byte payload
//==============================================================================
static void handle_submit_answer(int client_fd, MessageHeader *req, const char *payload) {
  printf("[Round1] SUBMIT_ANSWER payload size: %u bytes\n", req->length);
  
  // Accept 13-byte (old), 17-byte (new), or struct size
  if (req->length < 13) {
    printf("[Round1] Invalid SUBMIT_ANSWER payload size: got %u\n", req->length);
    send_json_response(client_fd, req, ERR_BAD_REQUEST,
      "{\"success\":false,\"error\":\"Invalid payload\"}");
    return;
  }

  uint32_t match_id = 0;
  uint32_t player_id = 0;
  uint32_t question_idx = 0;
  uint8_t choice = 0;
  uint32_t time_ms = 0;

  if (req->length == 13) {
    // Old format: match_id(4) + question_idx(4) + choice(1) + time_ms(4)
    memcpy(&match_id, payload, 4);
    memcpy(&question_idx, payload + 4, 4);
    choice = (uint8_t)payload[8];
    memcpy(&time_ms, payload + 9, 4);
    match_id = ntohl(match_id);
    question_idx = ntohl(question_idx);
    time_ms = ntohl(time_ms);
    player_id = 1; // default
  } else if (req->length == 17) {
    // New format: match_id(4) + player_id(4) + question_idx(4) + choice(1) + time_ms(4)
    memcpy(&match_id, payload, 4);
    memcpy(&player_id, payload + 4, 4);
    memcpy(&question_idx, payload + 8, 4);
    choice = (uint8_t)payload[12];
    memcpy(&time_ms, payload + 13, 4);
    match_id = ntohl(match_id);
    player_id = ntohl(player_id);
    question_idx = ntohl(question_idx);
    time_ms = ntohl(time_ms);
  } else {
    // Struct format (with padding)
    SubmitAnswerPayload data;
    memcpy(&data, payload, sizeof(data));
    match_id = ntohl(data.match_id);
    player_id = ntohl(data.player_id);
    question_idx = ntohl(data.question_idx);
    choice = data.choice;
    time_ms = ntohl(data.time_taken_ms);
  }

  printf("[Round1] Submit answer - match=%u player=%u idx=%u choice=%u time=%ums\n", 
         match_id, player_id, question_idx, choice, time_ms);

  // Lấy offset thực từ mảng random
  int actual_offset = 0;
  if (question_idx < (uint32_t)QUESTIONS_PER_ROUND && g_total_questions > 0) {
    actual_offset = g_random_indices[question_idx];
    printf("[Round1] Using random_indices[%u] = %d\n", question_idx, actual_offset);
  } else if (g_total_questions > 0) {
    actual_offset = (int)question_idx % g_total_questions;
    printf("[Round1] Using fallback offset=%d\n", actual_offset);
  }

  char qbuf[4096];
  int correct_index = 0;
  if (get_question_by_offset(actual_offset, qbuf, sizeof(qbuf), &correct_index) != 0) {
    printf("[Round1] WARNING: Could not get question for offset=%d, defaulting correct_index=0\n", actual_offset);
    correct_index = 0;
  }

  bool is_correct = (choice <= 3) && ((int)choice == correct_index);

  int score = 0;
  if (is_correct) {
    float time_sec = time_ms / 1000.0f;
    score = (int)(200 - (time_sec / 15.0f) * 100);
    if (score < 100) score = 100;
    score = (score / 10) * 10;
  }

  printf("[Round1] Answer result - correct_index=%d choice=%u is_correct=%s score=%d\n",
         correct_index, choice, is_correct ? "true" : "false", score);

  // Track player score
  int player_idx = find_player_index(client_fd);
  if (player_idx >= 0) {
    g_players[player_idx].score += score;
    printf("[Round1] Player %u total score: %d\n", player_id, g_players[player_idx].score);
  }

  char json[512];
  snprintf(json, sizeof(json),
    "{"
    "\"success\":true,"
    "\"match_id\":%u,"
    "\"player_id\":%u,"
    "\"question_idx\":%u,"
    "\"is_correct\":%s,"
    "\"score\":%d,"
    "\"correct_index\":%d,"
    "\"time_taken_ms\":%u"
    "}",
    match_id, player_id, question_idx,
    is_correct ? "true" : "false",
    score, correct_index, time_ms
  );

  send_json_response(client_fd, req, OP_S2C_ROUND1_RESULT, json);
}
//==============================================================================
// CASE 4: END ROUND (Player finished all questions)
//==============================================================================
static void handle_end_round(int client_fd, MessageHeader *req, const char *payload) {
  // Accept both 4-byte (old) and 8-byte (new) payloads
  if (req->length < 4) {
    send_json_response(client_fd, req, ERR_BAD_REQUEST,
      "{\"success\":false,\"error\":\"Invalid payload\"}");
    return;
  }

  uint32_t match_id = 0;
  uint32_t player_id = 0;
  memcpy(&match_id, payload, 4);
  match_id = ntohl(match_id);
  
  if (req->length >= 8) {
    memcpy(&player_id, payload + 4, 4);
    player_id = ntohl(player_id);
  }

  printf("[Round1] Player %u finished - match_id=%u\n", player_id, match_id);

  // Mark player as finished
  int player_idx = find_player_index(client_fd);
  if (player_idx >= 0 && !g_players[player_idx].is_finished) {
    g_players[player_idx].is_finished = true;
    g_finished_count++;
    printf("[Round1] Player %u marked finished (%d/%d)\n", 
           g_players[player_idx].player_id, g_finished_count, g_player_count);
  }

  // Send WAITING status to this player
  char waiting_json[256];
  snprintf(waiting_json, sizeof(waiting_json),
    "{\"success\":true,\"finished_count\":%d,\"player_count\":%d,\"waiting\":true}",
    g_finished_count, g_player_count);
  send_json_response(client_fd, req, OP_S2C_ROUND1_WAITING, waiting_json);

  // Check if all players finished
  if (g_finished_count >= g_player_count && g_player_count > 0) {
    printf("[Round1] All %d players finished! Sending final scoreboard...\n", g_player_count);
    
    // Build scoreboard with real scores
    char *scoreboard_json = build_scoreboard_json();
    printf("[Round1] Final scoreboard: %s\n", scoreboard_json);
    
    // Broadcast to all players
    broadcast_to_all_players(req, OP_S2C_ROUND1_ALL_FINISHED, scoreboard_json);
    free(scoreboard_json);
  }
}

//==============================================================================
// HANDLE PLAYER DISCONNECT - Called when a client socket closes
//==============================================================================
void handle_round1_disconnect(int client_fd) {
  int player_idx = find_player_index(client_fd);
  if (player_idx < 0) {
    printf("[Round1] Disconnect: client_fd=%d not found in player list\n", client_fd);
    return;
  }

  uint32_t player_id = g_players[player_idx].player_id;
  printf("[Round1] Player %u disconnected (fd=%d)\n", player_id, client_fd);

  // Build disconnect notification JSON
  char json[256];
  snprintf(json, sizeof(json), 
    "{\"player_id\":%u,\"name\":\"Player%u\",\"event\":\"disconnect\"}",
    player_id, player_id);

  // Broadcast to other players (NTF_PLAYER_LEFT = 701 = 0x02BD)
  MessageHeader ntf_header;
  memset(&ntf_header, 0, sizeof(ntf_header));
  ntf_header.magic = htons(MAGIC_NUMBER);
  ntf_header.version = PROTOCOL_VERSION;
  ntf_header.command = htons(0x02BD); // NTF_PLAYER_LEFT
  ntf_header.length = htonl(strlen(json));

  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_players[i].client_fd >= 0 && g_players[i].client_fd != client_fd) {
      send(g_players[i].client_fd, &ntf_header, sizeof(ntf_header), 0);
      send(g_players[i].client_fd, json, strlen(json), 0);
      printf("[Round1] Sent disconnect notification to player %u\n", g_players[i].player_id);
    }
  }

  // Mark as disconnected but keep player info for scoreboard
  g_players[player_idx].client_fd = -1;
  // Note: Don't clear player_id or score so scoreboard can still show them
  
  if (g_players[player_idx].is_ready && g_ready_count > 0) g_ready_count--;
  // Note: g_player_count tracks connected players
  if (g_player_count > 0) g_player_count--;

  printf("[Round1] Player disconnected. Remaining connected: %d players, %d ready\n", 
         g_player_count, g_ready_count);

  // If game is in progress and player count drops to 1, could end game
  if (g_game_started && g_player_count < 2) {
    printf("[Round1] Not enough players remaining (%d), game should end\n", g_player_count);
  }
}

//==============================================================================
// MAIN DISPATCHER
//==============================================================================
void handle_round1(int client_fd, MessageHeader *req_header, const char *payload) {
  printf("[Round1] cmd=0x%04X len=%u\n", req_header->command, req_header->length);

  switch (req_header->command) {
    case OP_C2S_ROUND1_READY:
      handle_ready(client_fd, req_header, payload);
      break;
    case OP_C2S_ROUND1_PLAYER_READY:
      handle_player_ready(client_fd, req_header, payload);
      break;
    case OP_C2S_ROUND1_GET_QUESTION:
      handle_get_question(client_fd, req_header, payload);
      break;
    case OP_C2S_ROUND1_ANSWER:
      handle_submit_answer(client_fd, req_header, payload);
      break;
    case OP_C2S_ROUND1_END:
    case OP_C2S_ROUND1_FINISHED:
      handle_end_round(client_fd, req_header, payload);
      break;
    default:
      printf("[Round1] Unknown command: 0x%04X\n", req_header->command);
      break;
  }
}