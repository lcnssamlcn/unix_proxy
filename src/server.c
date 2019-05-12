#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/**
 * default server port
 */
#define SERVER_PORT 3918
/**
 * maximum number of connections to this server at the same time
 */
#define MAX_CONNECTIONS 100
/**
 * maximum HTTP response length
 */
#define MAX_RESPONSE_LEN 65536

/**
 * server busy error message
 */
const char ERR_SERVER_BUSY[] = "Server is busy. Sorry.\n";

/**
 * server socket descriptor
 */
int server_sd = 0;

/**
 * thread 
 */
struct Thread {
    pthread_t thread;
    unsigned int id;
    int client_sd;
    struct sockaddr_in client;
};

/**
 * client-server connection threads
 */
struct Thread* threads[MAX_CONNECTIONS];
/**
 * current number of connections
 */
volatile int num_connections = 0;

/**
 * close the server and deallocate all resources
 */
void close_server(int status) {
    printf("closing server...\n");
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (threads[i] != NULL) {
            pthread_cancel(threads[i]->thread);
            close(threads[i]->client_sd);
        }
    }
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (threads[i] != NULL) {
            pthread_join(threads[i]->thread, NULL);
        }
    }
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (threads[i] != NULL) {
            free(threads[i]); threads[i] = NULL;
        }
    }
    close(server_sd);
    exit(status);
}

/**
 * event handler for SIGINT
 */
void SIGINT_handler(int signum) {
    close_server(0);
}

/**
 * client request handler
 * @param p_t client-server connection thread. It is castable with <i>struct Thread*</i>.
 */
void* request(void* p_t) {
    struct Thread* t = (struct Thread*) p_t;
    printf("Client %d: %s:%d\n", t->id, inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port));

    char response[MAX_RESPONSE_LEN + 1] = {0};

    ssize_t response_len = 1;
    while (response_len) {
        response_len = recv(t->client_sd, response, MAX_RESPONSE_LEN, 0);
        if (response_len > 0) {
            response[MAX_RESPONSE_LEN] = '\0';
            printf("received\n--------\n%s--------\nfrom %s:%d\n", response, inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port));
            memset(response, '\0', MAX_RESPONSE_LEN + 1);
        }
        else if (response_len == -1) {
            perror("Fail to receive message from client");
            close(t->client_sd);
            unsigned int tid = t->id;
            free(t); t = NULL; threads[tid] = NULL;
            num_connections--;
            exit(1);
        }
    }

    printf("client %s:%d disconnected\n", inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port));
    close(t->client_sd);
    unsigned int tid = t->id;
    free(t); t = NULL; threads[tid] = NULL;
    num_connections--;

    return 0;
}

int main() {
    signal(SIGINT, SIGINT_handler);

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        threads[i] = NULL;
    }

    server_sd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse_addr = 1;
    setsockopt(server_sd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

    if (server_sd == -1) {
        perror("Fail to create socket");
        exit(1);
    }

    struct sockaddr_in server;
    socklen_t saddr_len = sizeof(struct sockaddr_in);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = INADDR_ANY;
    memset(server.sin_zero, '0', 8);

    if (bind(server_sd, (struct sockaddr*) &server, saddr_len) == -1) {
        perror("Fail to bind static IP and port");
        close(server_sd);
        exit(1);
    }

    if (listen(server_sd, MAX_CONNECTIONS + 1) == -1) {
        perror("Fail to listen");
        close(server_sd);
        exit(1);
    }

    while (1) {
        int client_sd = 0;
        struct sockaddr_in client;

        if ((client_sd = accept(server_sd, (struct sockaddr*) &client, &saddr_len)) == -1) {
            perror("Fail to accept new client connection");
            close_server(1);
        }

        num_connections++;
        if (num_connections > MAX_CONNECTIONS) {
            send(client_sd, ERR_SERVER_BUSY, strlen(ERR_SERVER_BUSY), 0);
            close(client_sd);
            num_connections--;
            continue;
        }

        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (threads[i] == NULL) {
                threads[i] = malloc(sizeof(struct Thread));
                threads[i]->id = i;
                threads[i]->client_sd = client_sd;
                threads[i]->client = client;
                pthread_create(&threads[i]->thread, NULL, request, threads[i]);
                break;
            }
        }
    }

    close_server(0);

    return 0;
}
