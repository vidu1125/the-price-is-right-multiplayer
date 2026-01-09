#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <time.h>


#include "transport/socket_server.h"
#include "protocol/protocol.h"
#include "handlers/dispatcher.h"

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

fd_set master_set, read_set;
int listen_fd, max_fd;
volatile int g_running = 1;

//==============================================================================
// SERVER INITIALIZATION
//==============================================================================

int create_listening_socket() {
    struct sockaddr_in server_addr;
    int sockfd, opt = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

    FD_ZERO(&master_set);
    FD_SET(listen_fd, &master_set);
    max_fd = listen_fd;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sockfd = -1;
        clients[i].buffer_len = 0;
        clients[i].header_received = 0;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Socket server started on port %d\n", SERVER_PORT);
}

//==============================================================================
// MESSAGE PROCESSING  (DISPATCH ONLY)
//==============================================================================

void process_message(int client_idx, MessageHeader *header, char *payload) {
    int client_fd = clients[client_idx].sockfd;

    // DISPATCH ONLY — socket does NOT handle logic
    extern void dispatch_command(
        int client_fd,
        MessageHeader *header,
        const char *payload
    );

    dispatch_command(client_fd, header, payload);
}

void handle_client_data(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    int r;

    r = recv(client->sockfd,
             client->buffer + client->buffer_len,
             BUFF_SIZE - client->buffer_len,
             0);

    if (r <= 0) {
        close(client->sockfd);
        FD_CLR(client->sockfd, &master_set);
        client->sockfd = -1;
        client->buffer_len = 0;
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
            close(client->sockfd);
            FD_CLR(client->sockfd, &master_set);
            client->sockfd = -1;
            client->buffer_len = 0;
            return;
        }

        if (header.magic != MAGIC_NUMBER ||
            header.version != PROTOCOL_VERSION) {

            printf("[PROTO] invalid header: magic=0x%04x version=%u\n",
                header.magic, header.version);

            close(client->sockfd);
            FD_CLR(client->sockfd, &master_set);
            client->sockfd = -1;
            client->buffer_len = 0;
            return;
        }

        int total_len = sizeof(MessageHeader) + header.length;

        if (client->buffer_len < total_len)
            return;  // chưa đủ data → chờ recv tiếp

        char *payload = NULL;
        if (header.length > 0)
            payload = client->buffer + sizeof(MessageHeader);

        process_message(client_idx, &header, payload);

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
    struct timeval timeout;

    while (g_running) {
        read_set = master_set;

        timeout.tv_sec = SELECT_TIMEOUT;
        timeout.tv_usec = 0;

        int n = select(max_fd + 1, &read_set, NULL, NULL, &timeout);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        if (FD_ISSET(listen_fd, &read_set)) {
            int client_fd = accept(listen_fd, NULL, NULL);
            if (client_fd >= 0) {
                printf("Socket server started on port %d\n", SERVER_PORT);

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].sockfd == -1) {
                        clients[i].sockfd = client_fd;
                        clients[i].buffer_len = 0;
                        FD_SET(client_fd, &master_set);
                        if (client_fd > max_fd) max_fd = client_fd;
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sockfd != -1 &&
                FD_ISSET(clients[i].sockfd, &read_set)) {
                handle_client_data(i);
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

    if (listen_fd != -1)
        close(listen_fd);

    printf("Socket server shutdown\n");
}
