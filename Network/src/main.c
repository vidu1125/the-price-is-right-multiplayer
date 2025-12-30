#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

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
    printf("Press Ctrl+C to stop\n\n");

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
