#include "HTTPProxyResponse.h"
#include "globals.h"
#include "err_doc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/**
 * construct a new error response
 * @param http_ver http version
 * @param status_code HTTP error status code. NOTE that only status code defined in {@link HTTP_status_code} enum
 *                    are supported at currrent stage.
 * @param result resulting error response
 */
void HTTPProxyResponse_construct_err_response(const char* http_ver, const int status_code, struct HTTPProxyResponse* result) {
    if (http_ver != NULL)
        strcpy(result->http_ver, http_ver);
    else
        strcpy(result->http_ver, "HTTP/1.0");
    sprintf(result->status, "%d", status_code);
    int mapping = map_status_code(status_code);
    char* phrase = strchr(ERR_DOC_HEADING[mapping], ' ');
    if (phrase != NULL) {
        phrase++;
        strcpy(result->phrase, phrase);
    }
    else {
        strcpy(result->phrase, "Unknown");
    }
}

/**
 * write the headers to resulting response <i>result</i>
 * @param response <i>HTTPProxyResponse</i> instance containing all fields to construct the HTTP proxy response
 * @param result resulting HTTP proxy response
 */
void HTTPProxyResponse_write_headers(struct HTTPProxyResponse* response, char* result) {
    strcpy(result, response->http_ver);
    strcat(result, " ");
    strcat(result, response->status);
    strcat(result, " ");
    strcat(result, response->phrase);
    strcat(result, "\r\n\r\n");
}

/**
 * write the error document to resulting response <i>result</i>
 * @param response <i>HTTPProxyResponse</i> instance containing all fields to construct the HTTP proxy response
 * @param desc description of the error. It should be in HTML string format. See {@link ERR_DOC_DESC} for examples.
 *             You can pass NULL to use the default error description based on status code defined in {@link ERR_DOC_DESC}.
 * @param result resulting HTTP proxy response
 */
void HTTPProxyResponse_write_err_payload(struct HTTPProxyResponse* response, const char* desc, char* result) {
    char err_doc[MAX_BUFFER_LEN + 1] = {0};
    gen_err_doc(atoi(response->status), desc, err_doc);
    strcat(result, err_doc);
}
