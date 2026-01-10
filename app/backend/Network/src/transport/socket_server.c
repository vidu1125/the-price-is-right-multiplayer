#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>


#include "transport/socket_server.h"
#include "protocol/protocol.h"
#include "handlers/dispatcher.h"
#include "handlers/round1_handler.h"

// REMOVED: business / handler / db
// #include "../include/handler/client_manager.h"
// #include "../include/handler/room_manager.h"
// #include "../include/database.h"
// #include "../include/handler/message_handler.h"

//==============================================================================
// GLOBAL VARIABLES  (TRANSPORT-ONLY)
//==============================================================================

typedef struct {
    int sockfd;
    char buffer[BUFF_SIZE];
    int buffer_len;
    MessageHeader current_header;
    int header_received;
} ClientConnection;

ClientConnection clients[MAX_CLIENTS];

int listen_fd;
int epoll_fd;
struct epoll_event events[MAX_EVENTS];
volatile int g_running = 1;

//==============================================================================
// SERVER INITIALIZATION
//==============================================================================

// Helper to set non-blocking
void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return;
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int create_listening_socket() {
    struct sockaddr_in server_addr;
    int sockfd, opt = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Set listening socket to non-blocking
    set_nonblocking(sockfd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 10) < 0) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void initialize_server() {
    listen_fd = create_listening_socket();

    // Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }

    // Add listening socket to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN; // Available for read
    ev.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("epoll_ctl: listen_fd");
        exit(EXIT_FAILURE);
    }

    // Initialize client structures
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sockfd = -1;
        clients[i].buffer_len = 0;
        clients[i].header_received = 0;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Socket server started on port %d (EPOLL)\n", SERVER_PORT);
}

//==============================================================================
// MESSAGE PROCESSING  (DISPATCH ONLY)
//==============================================================================

void process_message(int client_idx, MessageHeader *header, char *payload) {
    int client_fd = clients[client_idx].sockfd;

    extern void dispatch_command(
        int client_fd,
        MessageHeader *header,
        const char *payload
    );

    dispatch_command(client_fd, header, payload);
}

// Helper to find client index by fd (needed because epoll gives us fd directly)
int get_client_index(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd == fd) return i;
    }
    return -1;
}

void handle_client_disconnect(int client_idx) {
    int fd = clients[client_idx].sockfd;
    if (fd == -1) return;

    printf("[Socket] Client disconnected: fd=%d\n", fd);
    
    // Notify round1 handler about disconnect
    handle_round1_disconnect(fd);
    
    // Remove from epoll - strictly speaking, closing the fd removes it from epoll automatically
    // but explicit removal is good practice if structure pointer was used.
    // Since we used fd in ev.data.fd, close() cleanups effectively.
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);

    close(fd);
    clients[client_idx].sockfd = -1;
    clients[client_idx].buffer_len = 0;
}

void handle_client_data(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    int r;

    r = recv(client->sockfd,
             client->buffer + client->buffer_len,
             BUFF_SIZE - client->buffer_len,
             0);

    if (r <= 0) {
        if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Spurious wakeup or non-blocking socket having no more data
            return;
        }
        handle_client_disconnect(client_idx);
        return;
    }

    client->buffer_len += r;

    while (client->buffer_len >= (int)sizeof(MessageHeader)) {

        MessageHeader header;
        memcpy(&header, client->buffer, sizeof(MessageHeader));

        header.magic    = ntohs(header.magic);
        header.command  = ntohs(header.command);
        header.reserved = ntohs(header.reserved);
        header.seq_num  = ntohl(header.seq_num);
        header.length   = ntohl(header.length);


        if (header.length > MAX_PAYLOAD_SIZE) {
            printf("[PROTO] payload too large: %u\n", header.length);
            handle_client_disconnect(client_idx);
            return;
        }

        if (header.magic != MAGIC_NUMBER ||
            header.version != PROTOCOL_VERSION) {

            printf("[PROTO] invalid header: magic=0x%04x version=%u\n",
                header.magic, header.version);

            handle_client_disconnect(client_idx);
            return;
        }

        int total_len = sizeof(MessageHeader) + header.length;

        if (client->buffer_len < total_len)
            return;  // not enough data

        char *payload = NULL;
        if (header.length > 0)
            payload = client->buffer + sizeof(MessageHeader);

        process_message(client_idx, &header, payload);

        // Check if client was disconnected during processing
        if (client->sockfd == -1) return;

        memmove(client->buffer,
                client->buffer + total_len,
                client->buffer_len - total_len);

        client->buffer_len -= total_len;
    }
}

// SIGNAL HANDLER (private)
void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\nReceived shutdown signal, stopping server...\n");
        g_running = 0;
    }
}


void forward_response(
    int client_fd,
    MessageHeader *req,
    uint16_t cmd,
    const char *payload,
    uint32_t payload_len
) {
    MessageHeader resp;
    memset(&resp, 0, sizeof(resp));

    resp.magic   = htons(MAGIC_NUMBER);
    resp.version = PROTOCOL_VERSION;
    resp.command = htons(cmd);
    resp.seq_num = htonl(req->seq_num);
    resp.length  = htonl(payload_len);

    send(client_fd, &resp, sizeof(resp), 0);
    if (payload_len > 0)
        send(client_fd, payload, payload_len, 0);
}
//==============================================================================
// MAIN LOOP
//==============================================================================

int main_loop() {
    int timeout_ms = -1; // Wait indefinitely for events

    while (g_running) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout_ms);
        
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listen_fd) {
                // Handle new connection
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
                
                if (client_fd == -1) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("accept");
                    }
                    continue;
                }

                // Make new socket non-blocking
                set_nonblocking(client_fd);

                // Find a slot in clients array
                int client_stored = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].sockfd == -1) {
                        clients[i].sockfd = client_fd;
                        clients[i].buffer_len = 0;
                        client_stored = 1;
                        
                        // Add to epoll
                        struct epoll_event ev;
                        ev.events = EPOLLIN | EPOLLRDHUP; // Read + Hang up detection
                        ev.data.fd = client_fd;
                        
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                            perror("epoll_ctl: add client");
                            close(client_fd);
                            clients[i].sockfd = -1;
                        } else {
                            printf("[Socket] New client connected: fd=%d\n", client_fd);
                        }
                        break;
                    }
                }

                if (!client_stored) {
                    printf("[Socket] Server full, rejecting client fd=%d\n", client_fd);
                    close(client_fd);
                }

            } else {
                // Handle client data
                int fd = events[n].data.fd;
                int idx = get_client_index(fd);
                
                if (idx != -1) {
                    // Check for errors or hangup
                    if (events[n].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                        handle_client_disconnect(idx);
                    } else if (events[n].events & EPOLLIN) {
                        handle_client_data(idx);
                    }
                }
            }
        }
    }

    return 0;
}

//==============================================================================
// SHUTDOWN
//==============================================================================

void shutdown_server() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != -1) {
            close(clients[i].sockfd);
        }
    }

    if (listen_fd != -1) {
        // remove from epoll before closing (optional but safe)
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, listen_fd, NULL);
        close(listen_fd);
    }
        
    if (epoll_fd != -1)
        close(epoll_fd);

    printf("Socket server shutdown\n");
}
