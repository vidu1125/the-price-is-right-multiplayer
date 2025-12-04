#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "../../protocol/packet_format.h"

#define PORT 8888

void* handle_client(void* arg) {
    int sock = *(int*)arg;
    free(arg);
    printf(" Client connected: %d\n", sock);
    
    char buffer[1024];
    while (recv(sock, buffer, sizeof(buffer), 0) > 0) {
        send(sock, buffer, strlen(buffer), 0);
    }
    
    close(sock);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    printf("═══════════════════════════════════════\n");
    printf("  The Price Is Right - Socket Server  \n");
    printf("═══════════════════════════════════════\n\n");
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);
    
    printf(" Listening on port %d\n\n", PORT);
    
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        pthread_t tid;
        int* sock = malloc(sizeof(int));
        *sock = new_socket;
        pthread_create(&tid, NULL, handle_client, sock);
        pthread_detach(tid);
    }
    
    return 0;
}
