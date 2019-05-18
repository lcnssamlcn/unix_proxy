#ifndef _HTTPHEADER_H_
#define _HTTPHEADER_H_

#include "globals.h"

/**
 * HTTP header
 */
struct HTTPHeader {
    /**
     * header name/key
     */
    char name[MAX_FIELD_LEN];
    /**
     * header value
     */
    char value[MAX_FIELD_LEN];
};

extern void HTTPHeader_construct(const char* header, struct HTTPHeader* result);
extern int HTTPHeader_is_header(const char* line);
extern void HTTPHeader_to_string(struct HTTPHeader* header, char* result, int appendCRLF);
extern struct HTTPHeader* HTTPHeader_find(struct HTTPHeader* headers, const unsigned int num_headers, const char* name);

#endif
