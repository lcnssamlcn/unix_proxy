#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "globals.h"
#include "HTTPProxyRequest.h"
// #include "HTTPResponse.h"

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
#define MAX_BUFFER_LEN 65536

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

    char proxy_request_raw[MAX_BUFFER_LEN + 1] = {0};

    ssize_t proxy_request_len = 1;
    while (proxy_request_len) {
        proxy_request_len = recv(t->client_sd, proxy_request_raw, MAX_BUFFER_LEN, 0);
        if (proxy_request_len > 0) {
            proxy_request_raw[MAX_BUFFER_LEN] = '\0';
#ifdef DEBUG
            printf("received\n--------\n%s--------\nfrom %s:%d\n\n", proxy_request_raw, inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port));
#endif
            struct HTTPProxyRequest proxy_request;
            memset(&proxy_request, '\0', sizeof(struct HTTPProxyRequest));
            HTTPProxyRequest_construct(proxy_request_raw, &proxy_request);
            char request[MAX_BUFFER_LEN + 1] = {0};
            HTTPProxyRequest_to_http_request(&proxy_request, request);
#ifdef DEBUG
            printf("sending request to remote server\n--------\n%s--------\n", request);
#endif
            struct addrinfo remote_server_hints;
            struct addrinfo* remote_server_addrinfos;
            memset(&remote_server_hints, 0, sizeof(struct addrinfo));
            remote_server_hints.ai_family = AF_INET;
            remote_server_hints.ai_socktype = SOCK_STREAM;
            char hostname[MAX_FIELD_LEN] = {0};
            char protocol[10] = {0};
            HTTPProxyRequest_get_hostname(&proxy_request, hostname);
            HTTPProxyRequest_get_protocol(&proxy_request, protocol);
            int ret;
            if ((ret = getaddrinfo(hostname, protocol, &remote_server_hints, &remote_server_addrinfos)) != 0) {
                fprintf(stderr, "Fail to do DNS lookup: %s\n", gai_strerror(ret));
                close(t->client_sd);
                unsigned int tid = t->id;
                free(t); t = NULL; threads[tid] = NULL;
                num_connections--;
                exit(1);
            }
            int remote_server_sd = 0;
            for (struct addrinfo* p = remote_server_addrinfos; p != NULL; p = p->ai_next) {
                if ((remote_server_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                    if (p->ai_next == NULL) {
                        perror("Fail to create socket to connect to remote server");
                        close(t->client_sd);
                        unsigned int tid = t->id;
                        free(t); t = NULL; threads[tid] = NULL;
                        num_connections--;
                        exit(1);
                    }
                    else {
                        continue;
                    }
                }
                if (connect(remote_server_sd, p->ai_addr, p->ai_addrlen) == -1) {
                    if (p->ai_next == NULL) {
                        perror("Fail to connect to remote server");
                        close(remote_server_sd);
                        close(t->client_sd);
                        unsigned int tid = t->id;
                        free(t); t = NULL; threads[tid] = NULL;
                        num_connections--;
                        exit(1);
                    }
                    else {
                        close(remote_server_sd);
                        continue;
                    }
                }
                break;
            }
            freeaddrinfo(remote_server_addrinfos);

            ssize_t sent = send(remote_server_sd, request, strlen(request), 0);
            if (sent > 0) {
                char response_raw[MAX_BUFFER_LEN + 1] = {0};
#ifdef DEBUG
                printf("response:\n--------\n");
#endif
                while (1) {
                    ssize_t recved = recv(remote_server_sd, response_raw, MAX_BUFFER_LEN, 0);
                    if (recved > 0) {
                        response_raw[MAX_BUFFER_LEN] = '\0';
#ifdef DEBUG
                        printf("%s", response_raw);
#endif
                        ssize_t sent2 = send(t->client_sd, response_raw, strlen(response_raw), 0);
                        if (sent2 == -1) {
                            perror("Fail to send response body to client browser");
                            close(remote_server_sd);
                            close(t->client_sd);
                            unsigned int tid = t->id;
                            free(t); t = NULL; threads[tid] = NULL;
                            num_connections--;
                            exit(1);
                        }
                        memset(response_raw, '\0', MAX_BUFFER_LEN + 1);
                    }
                    else if (recved == 0) {
                        break;
                    }
                    else {
                        perror("Fail to receive HTTP response from remote server");
                        close(remote_server_sd);
                        close(t->client_sd);
                        unsigned int tid = t->id;
                        free(t); t = NULL; threads[tid] = NULL;
                        num_connections--;
                        exit(1);
                    }
                }
#ifdef DEBUG
                printf("\n--------\n");
#endif
            }
            else if (sent == -1) {
                perror("Fail to send HTTP proxy request to remote server");
                close(remote_server_sd);
                close(t->client_sd);
                unsigned int tid = t->id;
                free(t); t = NULL; threads[tid] = NULL;
                num_connections--;
                exit(1);
            }
            close(remote_server_sd);
            memset(proxy_request_raw, '\0', MAX_BUFFER_LEN + 1);
        }
        else if (proxy_request_len == -1) {
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
