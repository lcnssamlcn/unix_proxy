#include "HTTPResponse.h"
#include <string.h>


void HTTPResponse_construct(const char* orig_response, struct HTTPResponse* result) {
    strcpy(result->http_ver, strtok((char*) orig_response, " "));
    strcpy(result->status, strtok(NULL, " "));
    strcpy(result->phrase, strtok(NULL, "\r\n"));
}
