#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <cjson/cJSON.h>

#include "transport/socket_server.h"
#include "db/core/db_client.h"   // 🔹 THÊM

//==============================================================================
// USAGE
//==============================================================================

static void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("\nOptions:\n");
    printf("  -h, --help      Show this help message\n");
    printf("\n");
}

//==============================================================================
// MAIN
//==============================================================================

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    printf("=============================================\n");
    printf("        NETWORK SOCKET SERVER START           \n");
    printf("=============================================\n");
    printf("Build Date: %s %s\n", __DATE__, __TIME__);
    printf("Version: ROUND1_RACE_CONDITION_FIX_v2\n");
    printf("Changes:\n");
    printf("  - Round1: bonus_triggered flag via output param\n");
    printf("  - Round1: Time-based scoring (170pts @ 4s)\n");
    printf("  - Questions per round: 1/1/1 (testing mode)\n");
    printf("=============================================\n");
    printf("Press Ctrl+C to stop\n\n");

    // Initialize random seed once
    srand(time(NULL));

    // =====================================================
    // 🔹 INIT DB CLIENT (CHỈ 1 LẦN)
    // =====================================================
    printf("[DB] init...\n");
    if (db_client_init() != DB_OK) {
        fprintf(stderr, "[DB] init FAILED\n");
        return 1;
    }
    printf("[DB] init OK\n");
    
    if (db_ping() == 0) {
        printf("[DB] Supabase reachable\n");
    } else {
        printf("[DB] Supabase NOT reachable\n");
    }

    // =====================================================
    // 🔹 CLEANUP ZOMBIE ROOMS FROM PREVIOUS RUN
    // =====================================================
    printf("[DB] Cleaning zombie rooms from previous run...\n");
    
    // Close all rooms that were waiting or playing
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "status", "closed");
    cJSON *response = NULL;
    
    db_error_t rc = db_patch("rooms", "status IN ('waiting', 'playing')", payload, &response);
    
    if (rc == DB_OK) {
        printf("[DB] Closed zombie rooms\n");
    } else {
        printf("[DB] Warning: Failed to close zombie rooms (rc=%d)\n", rc);
    }
    
    cJSON_Delete(payload);
    if (response) cJSON_Delete(response);
    
    // Clear all room members
    response = NULL;
    rc = db_delete("room_members", "room_id > 0", &response);
    
    if (rc == DB_OK) {
        printf("[DB] Cleared room members\n");
    } else {
        printf("[DB] Warning: Failed to clear room members (rc=%d)\n", rc);
    }
    
    if (response) cJSON_Delete(response);
    printf("[DB] Cleanup complete\n");

    initialize_server();
    main_loop();
    shutdown_server();

    // =====================================================
    // 🔹 CLEANUP DB CLIENT
    // =====================================================
    db_client_cleanup();

    printf("\nServer stopped gracefully\n");
    return 0;
}
