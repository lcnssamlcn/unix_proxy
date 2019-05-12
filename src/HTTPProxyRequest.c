#include "globals.h"
#include "HTTPProxyRequest.h"
#include <string.h>


/**
 * constructor of <i>HTTPProxyRequest</i>
 * @param orig_request original raw HTTP request
 * @param result the new <i>HTTPProxyRequest</i> instance will be saved here
 */
void HTTPProxyRequest_construct(const char* orig_request, struct HTTPProxyRequest* result) {
    strcpy(result->method, strtok((char*) orig_request, " "));
    strcpy(result->url, strtok(NULL, " "));
    strcpy(result->http_ver, strtok(NULL, "\r\n"));
}

/**
 * convert HTTP proxy request to HTTP request
 * @param request current <i>HTTPProxyRequest</i> instance
 * @param result the resulting HTTP request will be saved here
 */
void HTTPProxyRequest_to_http_request(struct HTTPProxyRequest* request, char* result) {
    char rel_uri[MAX_FIELD_LEN] = {0};
    char hostname[MAX_FIELD_LEN] = {0};
    HTTPProxyRequest_get_rel_uri(request, rel_uri);
    HTTPProxyRequest_get_hostname(request, hostname);
    strcpy(result, request->method);
    strcat(result, " ");
    strcat(result, rel_uri);
    strcat(result, " ");
    strcat(result, request->http_ver);
    strcat(result, "\r\n");
    strcat(result, "Host: ");
    strcat(result, hostname);
    strcat(result, "\r\n\r\n");
}

/**
 * get the protocol of the URL stated in the <i>request</i>, e.g. "http"
 * @param request current <i>HTTPProxyRequest</i> instance
 * @param the resulting protocol will be saved here
 */
void HTTPProxyRequest_get_protocol(struct HTTPProxyRequest* request, char* result) {
    strncpy(result, request->url, strchr(request->url, ':') - request->url);
}

/**
 * get the hostname of the URL stated in the <i>request</i>, e.g. "www.example.com"
 * @param request current <i>HTTPProxyRequest</i> instance
 * @param the resulting hostname will be saved here
 */
void HTTPProxyRequest_get_hostname(struct HTTPProxyRequest* request, char* result) {
    char* hostname_start = strchr(request->url, ':') + 3;
    char* uri_start = strchr(hostname_start, '/');
    strncpy(result, hostname_start, uri_start - hostname_start);
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
