#ifndef _HTTPPROXYREQUEST_H_
#define _HTTPPROXYREQUEST_H_

#include "globals.h"
#include "HTTPHeader.h"

/**
 * HTTP proxy request representaiton
 */
struct HTTPProxyRequest {
    /**
     * HTTP method, e.g. "GET"
     */
    char method[10];
    /**
     * URL, e.g. "www.example.com"
     */
    char url[MAX_FIELD_LEN];
    /**
     * HTTP version, e.g. "HTTP/1.1"
     */
    char http_ver[10];
    /**
     * HTTP headers
     */
    struct HTTPHeader headers[100];
    /**
     * total number of HTTP headers received
     */
    unsigned int num_headers;
    /**
     * query string, only available in POST request
     */
    char query_string[1024];
};

extern void HTTPProxyRequest_construct(const char* orig_request, struct HTTPProxyRequest* result);
extern void HTTPProxyRequest_to_http_request(struct HTTPProxyRequest* request, char* result);
extern void HTTPProxyRequest_get_protocol(struct HTTPProxyRequest* request, char* result);
extern void HTTPProxyRequest_get_hostname(struct HTTPProxyRequest* request, char* result);
extern void HTTPProxyRequest_get_rel_uri(struct HTTPProxyRequest* request, char* result);

#endif
