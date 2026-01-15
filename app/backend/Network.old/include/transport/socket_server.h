#ifndef SERVER_H
#define SERVER_H


#include <sys/epoll.h>
#include <time.h>
#include <stdint.h>
#include "protocol/protocol.h"

#define SERVER_PORT     5500
#define MAX_CLIENTS     64
#define MAX_EVENTS      64



// Server lifecycle
void initialize_server(void);
int create_listening_socket(void);
int main_loop(void);
void shutdown_server(void);
void signal_handler(int signum);


// Message processing
void handle_client_data(int client_idx);
void process_message(int client_idx, MessageHeader *header, char *payload);



#endif // SOCKET_SERVER_H
