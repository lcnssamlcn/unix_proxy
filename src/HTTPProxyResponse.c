#include "HTTPProxyResponse.h"
#include <string.h>


/**
 * construct the HTTP proxy response in string form
 * @param response <i>HTTPProxyResponse</i> instance containing all fields to construct the HTTP proxy response
 * @param result resulting HTTP proxy response
 */
void HTTPProxyResponse_to_string(struct HTTPProxyResponse* response, char* result) {
    strcpy(result, response->http_ver);
    strcat(result, " ");
    strcat(result, response->status);
    strcat(result, " ");
    strcat(result, response->phrase);
    strcat(result, "\r\n\r\n");
}
