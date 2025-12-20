#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "../include/server.h"
#include "../include/client_manager.h"  // ✅ THÊM DÒNG NÀY

//==============================================================================
// SIGNAL HANDLER
//==============================================================================

void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\nReceived shutdown signal, stopping server...\n");
        g_running = 0;
    }
}

//==============================================================================
// USAGE
//==============================================================================

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("\nOptions:\n");
    printf("  -q, --quiet     Quiet mode (only errors)\n");
    printf("  -n, --normal    Normal mode (default - connections + important events)\n");
    printf("  -v, --verbose   Verbose mode (all commands and details)\n");
    printf("  -h, --help      Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s              # Run with normal logging\n", prog_name);
    printf("  %s -q           # Run with minimal logging\n", prog_name);
    printf("  %s --verbose    # Run with full logging\n", prog_name);
    printf("\n");
}

//==============================================================================
// MAIN
//==============================================================================

int main(int argc, char *argv[]) {
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            g_log_level = LOG_QUIET;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--normal") == 0) {
            g_log_level = LOG_NORMAL;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            g_log_level = LOG_VERBOSE;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║              GAME SERVER STARTING                                  ║\n");
    printf("╚════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    initialize_server();
    
    printf("\n");
    printf("✅ Server initialized successfully\n");
    printf("📊 Active clients: %d/%d\n", get_active_client_count(), MAX_CLIENTS);
    printf("\n");
    printf("Press Ctrl+C to stop\n");
    printf("════════════════════════════════════════════════════════════════════\n\n");
    
    main_loop();
    
    shutdown_server();
    
    return 0;
}