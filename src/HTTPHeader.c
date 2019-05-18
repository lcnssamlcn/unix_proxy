#include "HTTPHeader.h"
#include <stdio.h>
#include <string.h>


/**
 * construct a new HTTP header
 * @param header raw HTTP header
 * @param result new HTTP header
 */
void HTTPHeader_construct(const char* header, struct HTTPHeader* result) {
    sscanf(header, "%[^:]: %[^\n]", result->name, result->value);
}

/**
 * check if the line declares a HTTP header
 * @return 1 if so; otherwise 0
 */
int HTTPHeader_is_header(const char* line) {
    return strchr(line, ':') != NULL;
}

/**
 * convert HTTP header to string
 * @param header current <i>HTTPHeader</i> instance
 * @param result resulting HTTP header in string form
 * @param appendCRLF if it is 1, it will append &lt;CR&gt;&lt;LF&gt; at the end of the resulting string.
 */
void HTTPHeader_to_string(struct HTTPHeader* header, char* result, int appendCRLF) {
    strcpy(result, header->name);
    strcat(result, ": ");
    strcat(result, header->value);
    if (appendCRLF)
        strcat(result, "\r\n");
}

/**
 * find HTTP header by header name
 * @param headers array of <i>HTTPHeader</i>s to search
 * @param num_headers total number of headers in <i>headers</i>
 * @param name header name to search for
 * @return resulting HTTP header
 */
struct HTTPHeader* HTTPHeader_find(struct HTTPHeader* headers, const unsigned int num_headers, const char* name) {
    int i;
    for (i = 0; i < num_headers; i++) {
        if (strcmp(headers[i].name, name) == 0)
            return &headers[i];
    }
    return NULL;
}
