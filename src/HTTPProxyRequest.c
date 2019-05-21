#include "globals.h"
#include "HTTPHeader.h"
#include "HTTPProxyRequest.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/**
 * constructor of <i>HTTPProxyRequest</i>
 * @param orig_request original raw HTTP request
 * @param result the new <i>HTTPProxyRequest</i> instance will be saved here
 * @return 1 if the <i>orig_request</i> is valid; 0 otherwise
 */
int HTTPProxyRequest_construct(const char* orig_request, struct HTTPProxyRequest* result) {
    char* orig_request_copy = malloc(sizeof(char) * (strlen(orig_request) + 1));
    strcpy(orig_request_copy, orig_request);
    char* token = strtok(orig_request_copy, " ");
    if (token == NULL)
        return 0;
    strcpy(result->method, token);
    token = strtok(NULL, " ");
    if (token == NULL)
        return 0;
    strcpy(result->url, token);
    token = strtok(NULL, "\r\n");
    if (token == NULL)
        return 0;
    strcpy(result->http_ver, token);
    unsigned int num_headers = 0;
    while (1) {
        char* line = strtok(NULL, "\r\n");
        if (line == NULL)
            break;
        if (HTTPHeader_is_header(line)) {
            HTTPHeader_construct(line, &result->headers[num_headers]);
            // char header_raw[512] = {0};
            // HTTPHeader_to_string(&result->headers[num_headers], header_raw, 0);
            // printf("header: %s\n", header_raw);
            num_headers++;
        }
        else {
            strcpy(result->query_string, line);
        }
    }
    result->num_headers = num_headers;
    free(orig_request_copy); orig_request_copy = NULL;
    return 1;
}

/**
 * add header received from client browser to construct a new HTTP request
 * @param request current <i>HTTPProxyRequest</i> instance
 * @param header_name header to add
 * @param result the resulting HTTP request will be saved here
 */
void HTTPProxyRequest_add_header(struct HTTPProxyRequest* request, const char* header_name, char* result) {
    char header_raw[512] = {0};
    struct HTTPHeader* header = HTTPHeader_find(request->headers, request->num_headers, header_name);
    if (header != NULL)
        HTTPHeader_to_string(header, header_raw, 1);
    strcat(result, header_raw);
}

/**
 * convert HTTP proxy request to HTTP request
 * @param request current <i>HTTPProxyRequest</i> instance
 * @param result the resulting HTTP request will be saved here
 */
void HTTPProxyRequest_to_http_request(struct HTTPProxyRequest* request, char* result) {
    strcpy(result, request->method);
    strcat(result, " ");
    if (strcmp(request->method, "CONNECT") == 0) {
        strcat(result, request->url);
    }
    else {
        char rel_uri[MAX_FIELD_LEN] = {0};
        HTTPProxyRequest_get_rel_uri(request, rel_uri);
        strcat(result, rel_uri);
    }
    strcat(result, " ");
    strcat(result, request->http_ver);
    strcat(result, "\r\n");
    HTTPProxyRequest_add_header(request, "Host", result);
    if (strcmp(request->method, "CONNECT") == 0) {
        strcat(result, "Connection: ");
        struct HTTPHeader* header = HTTPHeader_find(request->headers, request->num_headers, "Proxy-Connection");
        if (header != NULL) {
            strcat(result, header->value);
            strcat(result, "\r\n");
        }
        else
            HTTPProxyRequest_add_header(request, "Connection", result);
    }
    else {
        HTTPProxyRequest_add_header(request, "Connection", result);
        HTTPProxyRequest_add_header(request, "Authorization", result);
    }
    if (strcmp(request->method, "POST") == 0) {
        HTTPProxyRequest_add_header(request, "Content-Type", result);
        HTTPProxyRequest_add_header(request, "Content-Length", result);
        strcat(result, "\r\n");
        strcat(result, request->query_string);
    }
    else
        strcat(result, "\r\n");
}

/**
 * get the protocol of the URL stated in the <i>request</i>, e.g. "http"
 * @param request current <i>HTTPProxyRequest</i> instance
 * @param the resulting protocol will be saved here
 */
void HTTPProxyRequest_get_protocol(struct HTTPProxyRequest* request, char* result) {
    if (strcmp(request->method, "CONNECT") == 0)
        strcpy(result, "https");
    else
        strcpy(result, "http");
}

/**
 * get the hostname of the URL stated in the <i>request</i>, e.g. "www.example.com"
 * @param request current <i>HTTPProxyRequest</i> instance
 * @param the resulting hostname will be saved here
 */
void HTTPProxyRequest_get_hostname(struct HTTPProxyRequest* request, char* result) {
    struct HTTPHeader* host = HTTPHeader_find(request->headers, request->num_headers, "Host");
    if (host != NULL) {
        char* port = strchr(host->value, ':');
        if (port != NULL)
            strncpy(result, host->value, port - host->value);
        else
            strcpy(result, host->value);
    }
}

/**
 * get the relative URI path of the URL stated in the <i>request</i>, e.g. "/index.html"
 * @param request current <i>HTTPProxyRequest</i> instance
 * @param the resulting relative URI path will be saved here
 */
void HTTPProxyRequest_get_rel_uri(struct HTTPProxyRequest* request, char* result) {
    char* uri_start = strchr(strchr(request->url, ':') + 3, '/');
    strcpy(result, uri_start);
}
