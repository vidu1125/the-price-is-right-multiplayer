#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "transport/socket_server.h"   

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

    initialize_server();  
    main_loop();          
    shutdown_server();    

    printf("\nServer stopped gracefully\n");
    return 0;
}
