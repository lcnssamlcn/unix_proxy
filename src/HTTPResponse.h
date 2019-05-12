#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

struct HTTPResponse {
    char http_ver[10];
    char status[4];
    char phrase[100];
};

extern void HTTPResponse_construct(const char* orig_response, struct HTTPResponse* result);

#endif
