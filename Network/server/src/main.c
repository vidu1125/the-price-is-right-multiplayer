// Network/server/src/main.c
/**
 * Main Server - TCP Socket Server with select() I/O Multiplexing
 * Thin transport layer - forwards requests to Python Backend via IPC
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "session.h"
#include "backend_bridge.h"
#include "packet_handler.h"
#include "router.h"
#include "../../protocol/packet_format.h"
#include "../../protocol/commands.h"

#define PORT 8888
#define MAX_CLIENTS 100

// Global flag for graceful shutdown
static volatile int running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

int setup_server(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return -1;
    }
    
    // Bind
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }
    
    // Listen
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }
    
    return server_fd;
}

bool process_client_packet(int client_fd) {
    // Read header
    MessageHeader header;
    if (!recv_packet_header(client_fd, &header)) {
        return false;  // Connection closed or error
    }
    
    uint16_t command = packet_get_command(&header);
    uint32_t length = packet_get_length(&header);
    
    // Read payload
    void* payload = NULL;
    if (length > 0) {
        if (!recv_packet_payload(client_fd, length, &payload)) {
            return false;
        }
    }
    
    // Route to handler
    route_command(command, client_fd, payload, length);
    
    if (payload) {
        free(payload);
    }
    
    return true;
}

int main() {
    printf("═══════════════════════════════════════\n");
    printf("  The Price Is Right - Socket Server  \n");
    printf("  Network Layer (C)                    \n");
    printf("═══════════════════════════════════════\n\n");
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);  // Ignore broken pipe
    
    // Initialize session manager
    if (!session_manager_init()) {
        fprintf(stderr, "Failed to initialize session manager\n");
        return 1;
    }
    
    // Connect to Python backend
    printf("[Init] Connecting to Python backend...\n");
    if (!backend_bridge_init(NULL)) {
        fprintf(stderr, "❌ Failed to connect to backend\n");
        fprintf(stderr, "   Make sure Python backend is running!\n");
        return 1;
    }
    
    // Setup server socket
    int server_fd = setup_server(PORT);
    if (server_fd < 0) {
        fprintf(stderr, "❌ Failed to setup server\n");
        backend_bridge_cleanup();
        return 1;
    }
    
    printf("✅ Listening on port %d\n\n", PORT);
    printf("Ready to accept connections...\n\n");
    
    // Setup select()
    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    int max_fd = server_fd;
    
    // Main loop
    while (running) {
        read_set = master_set;
        
        // Timeout for graceful shutdown check
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(max_fd + 1, &read_set, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (running) {
                perror("select");
            }
            continue;
        }
        
        if (activity == 0) {
            continue;  // Timeout
        }
        
        // Check for new connection
        if (FD_ISSET(server_fd, &read_set)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (client_fd < 0) {
                perror("accept");
                continue;
            }
            
            FD_SET(client_fd, &master_set);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            
            // Create session
            if (!session_create(client_fd)) {
                fprintf(stderr, "Failed to create session\n");
                close(client_fd);
                FD_CLR(client_fd, &master_set);
                continue;
            }
            
            printf("✅ Client connected: fd=%d from %s:%d\n",
                   client_fd,
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));
        }
        
        // Check existing connections
        for (int fd = 0; fd <= max_fd; fd++) {
            if (fd == server_fd) continue;
            
            if (FD_ISSET(fd, &read_set)) {
                if (!process_client_packet(fd)) {
                    // Connection closed
                    printf("❌ Client disconnected: fd=%d\n", fd);
                    close(fd);
                    FD_CLR(fd, &master_set);
                    session_destroy(fd);
                }
            }
        }
    }
    
    // Cleanup
    printf("\n\n🛑 Shutting down...\n");
    close(server_fd);
    session_cleanup_all();
    backend_bridge_cleanup();
    printf("✅ Server stopped\n");
    
    return 0;
}
