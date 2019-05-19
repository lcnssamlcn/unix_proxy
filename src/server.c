#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "globals.h"
#include "HTTPProxyRequest.h"
#include "HTTPProxyResponse.h"

/**
 * default server port
 */
#define SERVER_PORT 3918
/**
 * maximum number of client-server connections to handle at the same time
 */
#define MAX_CONNECTIONS 500
/**
 * maximum HTTP response/request length
 */
#define MAX_BUFFER_LEN 16384

/**
 * server busy error message
 */
const char ERR_SERVER_BUSY[] = "Server is busy. Sorry.\n";

/**
 * server socket descriptor
 */
int server_sd = 0;

/**
 * client thread struct maintaining the state of each client-server connection
 */
struct Thread {
    /**
     * thread
     */
    pthread_t thread;
    /**
     * index in {@link threads}
     */
    unsigned int id;
    /**
     * client's socket descriptor
     */
    int client_sd;
    /**
     * client's IP address
     */
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
 * mini struct storing client and remote server socket descriptors
 */
struct sds {
    int client_sd;
    int remote_server_sd;
};

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
 * deallocate all resources generated in client-server thread
 * @param t client-server thread
 * @param client_sd client socket descriptor to deallocate. You can pass -1 to cancel deallocation of this socket.
 * @param remote_server_sd remote server socket descriptor. You can pass -1 to cancel deallocation of this socket.
 */
void deallocate_thread(struct Thread* t, int client_sd, int remote_server_sd) {
    printf("client %s:%d disconnected\n", inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port));
    if (client_sd != -1)
        close(client_sd);
    if (remote_server_sd != -1)
        close(remote_server_sd);
    unsigned int tid = t->id;
    free(t); t = NULL; threads[tid] = NULL;
    num_connections--;
    pthread_exit(NULL);
}

/**
 * connect to the remote server
 * @param t client-server thread who wants to initiate the connection to the remote server
 * @param hostname hostname of the remote server (without port), e.g. "www.example.com"
 * @param protocol internet protocol to use, e.g. "http", "https"
 */
int connect_remote_server(struct Thread* t, const char* hostname, const char* protocol) {
    struct addrinfo remote_server_hints;
    struct addrinfo* remote_server_addrinfos;
    memset(&remote_server_hints, 0, sizeof(struct addrinfo));
    remote_server_hints.ai_family = AF_INET;
    remote_server_hints.ai_socktype = SOCK_STREAM;
#ifdef DEBUG
    printf("--------\nhostname: %s\nprotocol: %s\n--------\n", hostname, protocol);
#endif

    int ret;
    if ((ret = getaddrinfo(hostname, protocol, &remote_server_hints, &remote_server_addrinfos)) != 0) {
        fprintf(stderr, "Fail to do DNS lookup: %s\n", gai_strerror(ret));
        deallocate_thread(t, t->client_sd, -1);
    }
    int remote_server_sd = 0;
    for (struct addrinfo* p = remote_server_addrinfos; p != NULL; p = p->ai_next) {
        if ((remote_server_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            if (p->ai_next == NULL) {
                perror("Fail to create socket to connect to remote server");
                deallocate_thread(t, t->client_sd, -1);
            }
            else {
                continue;
            }
        }
        if (connect(remote_server_sd, p->ai_addr, p->ai_addrlen) == -1) {
            if (p->ai_next == NULL) {
                perror("Fail to connect to remote server");
                deallocate_thread(t, t->client_sd, remote_server_sd);
            }
            else {
                close(remote_server_sd);
                continue;
            }
        }
        break;
    }
    freeaddrinfo(remote_server_addrinfos);

    return remote_server_sd;
}

/**
 * forward client's HTTP request to the remote server
 * @param t client-server thread
 * @param remote_server_sd remote server socket descriptor
 * @param request HTTP request
 */
void forward_HTTP(struct Thread* t, int remote_server_sd, const char* request) {
#ifdef DEBUG
    printf("sending request from %s:%d to remote server\n--------\n%s--------\n", inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port), request);
#endif
    ssize_t sent = send(remote_server_sd, request, strlen(request), 0);
    if (sent > 0) {
        unsigned char response_raw[MAX_BUFFER_LEN + 1] = {0};
#ifdef DEBUG
        printf("response from %s:%d\n--------\n", inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port));
#endif
        while (1) {
            ssize_t recved = recv(remote_server_sd, response_raw, MAX_BUFFER_LEN, 0);
            if (recved > 0) {
                response_raw[MAX_BUFFER_LEN] = '\0';
#ifdef DEBUG
                printf("%s", response_raw);
#endif
                ssize_t sent2 = send(t->client_sd, response_raw, recved, 0);
                if (sent2 == -1) {
                    perror("Fail to send response body to client browser");
                    deallocate_thread(t, t->client_sd, remote_server_sd);
                }
                memset(response_raw, '\0', MAX_BUFFER_LEN + 1);
            }
            else if (recved == 0) {
                break;
            }
            else {
                perror("Fail to receive HTTP response from remote server");
                deallocate_thread(t, t->client_sd, remote_server_sd);
            }
        }
#ifdef DEBUG
        printf("\n--------\n");
#endif
    }
    else if (sent == -1) {
        perror("Fail to send HTTP proxy request to remote server");
        deallocate_thread(t, t->client_sd, remote_server_sd);
    }
    close(remote_server_sd);
}

/**
 * forward HTTPS packets from client to remote server
 * @param p_sds socket descriptors in type {@link sds}.
 */
void* forward_HTTPS_client_packets(void* p_sds) {
    struct sds* sds_ = (struct sds*) p_sds;
    while (1) {
        char buffer[MAX_BUFFER_LEN + 1] = {0};
        ssize_t recved = recv(sds_->client_sd, buffer, MAX_BUFFER_LEN, 0);
        if (recved <= 0)
            break;
        ssize_t sent = send(sds_->remote_server_sd, buffer, recved, 0);
        if (sent <= 0)
            break;
    }

    return 0;
}

/**
 * forward HTTPS packets from remote server to the client
 * @param p_sds socket descriptors in type {@link sds}.
 */
void* forward_HTTPS_remote_server_packets(void* p_sds) {
    struct sds* sds_ = (struct sds*) p_sds;
    while (1) {
        char buffer[MAX_BUFFER_LEN + 1] = {0};
        ssize_t recved = recv(sds_->remote_server_sd, buffer, MAX_BUFFER_LEN, 0);
        if (recved <= 0)
            break;
        ssize_t sent = send(sds_->client_sd, buffer, recved, 0);
        if (sent <= 0)
            break;
    }

    return 0;
}

/**
 * forward client's HTTPS request to the remote server
 * @param t client-server thread
 * @param remote_server_sd remote server socket descriptor
 * @param http_ver HTTP version
 */
void forward_HTTPS(struct Thread* t, int remote_server_sd, char* http_ver) {
    char proxy_response_raw[MAX_BUFFER_LEN] = {0};
    struct HTTPProxyResponse proxy_response;
    strcpy(proxy_response.http_ver, http_ver);
    strcpy(proxy_response.status, "200");
    strcpy(proxy_response.phrase, "Connection established");
    HTTPProxyResponse_to_string(&proxy_response, proxy_response_raw);
#ifdef DEBUG
    printf("proxy response to client %s:%d\n--------\n%s--------\n", inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port), proxy_response_raw);
#endif
    ssize_t sent = send(t->client_sd, proxy_response_raw, strlen(proxy_response_raw), 0);
    if (sent == -1) {
        perror("Fail to send HTTP proxy response to client");
        deallocate_thread(t, t->client_sd, remote_server_sd);
    }

    struct sds sds_;
    sds_.client_sd = t->client_sd;
    sds_.remote_server_sd = remote_server_sd;
    pthread_t client_tunnel_t;
    pthread_t remote_server_tunnel_t;
    pthread_create(&client_tunnel_t, NULL, forward_HTTPS_client_packets, &sds_);
    pthread_create(&remote_server_tunnel_t, NULL, forward_HTTPS_remote_server_packets, &sds_);
    pthread_join(client_tunnel_t, NULL);
    pthread_join(remote_server_tunnel_t, NULL);
}

/**
 * client request handler
 * @param p_t client-server connection thread. It is castable with <i>struct Thread*</i>.
 */
void* request(void* p_t) {
    struct Thread* t = (struct Thread*) p_t;
    printf("Client %d: %s:%d\n", t->id, inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port));

    char proxy_request_raw[MAX_BUFFER_LEN + 1] = {0};

    ssize_t proxy_request_len = recv(t->client_sd, proxy_request_raw, MAX_BUFFER_LEN, 0);
    if (proxy_request_len > 0) {
        proxy_request_raw[MAX_BUFFER_LEN] = '\0';
#ifdef DEBUG
        printf("received\n--------\n%s--------\nfrom %s:%d\n\n", proxy_request_raw, inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port));
#endif
        struct HTTPProxyRequest proxy_request;
        int isValid = HTTPProxyRequest_construct(proxy_request_raw, &proxy_request);
        if (!isValid) {
            fprintf(stderr, "Unknown request format.\n");
            deallocate_thread(t, t->client_sd, -1);
        }
        char request[MAX_BUFFER_LEN + 1] = {0};
        HTTPProxyRequest_to_http_request(&proxy_request, request);
        char hostname[MAX_FIELD_LEN] = {0};
        char protocol[10] = {0};
        HTTPProxyRequest_get_hostname(&proxy_request, hostname);
        HTTPProxyRequest_get_protocol(&proxy_request, protocol);
        int remote_server_sd = connect_remote_server(t, hostname, protocol);
        if (strcmp(proxy_request.method, "CONNECT") == 0) {
            forward_HTTPS(t, remote_server_sd, proxy_request.http_ver);
        }
        else {
            forward_HTTP(t, remote_server_sd, request);
        }
    }
    else if (proxy_request_len == -1) {
        perror("Fail to receive message from client");
        deallocate_thread(t, t->client_sd, -1);
    }

    printf("client %s:%d disconnected\n", inet_ntoa(t->client.sin_addr), ntohs(t->client.sin_port));
    deallocate_thread(t, t->client_sd, -1);

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
