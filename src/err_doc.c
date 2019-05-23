#include "globals.h"
#include "err_doc.h"
#include <string.h>


/**
 * map the HTTP status code to {@link HTTP_status_code} enum
 * @param status_code HTTP error status code. NOTE that only status code defined in {@link HTTP_status_code} enum
 *                    are supported at currrent stage. If you pass an unsupported status code, 500 error will be used.
 * @return the mapped result
 */
int map_status_code(const int status_code) {
    switch (status_code) {
        case 400:
            return BAD_REQUEST;
        case 404:
            return NOT_FOUND;
        case 500:
            return INTERNAL_SERVER_ERROR;
        case 501:
            return NOT_IMPLEMENTED;
        case 502:
            return BAD_GATEWAY;
        case 503:
            return SERVICE_UNAVAILABLE;
    }
    return INTERNAL_SERVER_ERROR;
}

/**
 * generate an error document
 * @param status_code HTTP error status code. NOTE that only status code defined in {@link HTTP_status_code} enum
 *                    are supported at currrent stage.
 * @param desc description of the error. It should be in HTML string format. See {@link ERR_DOC_DESC} for examples.
 *             You can pass NULL to use the default error description based on status code defined in {@link ERR_DOC_DESC}.
 * @param result resulting error document
 */
void gen_err_doc(const int status_code, const char* desc, char* result) {
    int mapping = map_status_code(status_code);
    char err_doc[MAX_BUFFER_LEN + 1] = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
            "<meta charset='UTF-8' />\n";
    strcat(err_doc, "<title>");
    strcat(err_doc, ERR_DOC_HEADING[mapping]);
    strcat(err_doc, "</title>\n");
    strcat(
        err_doc, 
        "</head>\n"
        "<body>\n"
            "<h1>"
    );
    strcat(err_doc, ERR_DOC_HEADING[mapping]);
    strcat(err_doc, "</h1>\n");
    if (desc == NULL)
        strcat(err_doc, ERR_DOC_DESC[mapping]);
    else {
        strcat(err_doc, desc);
    }
    strcat(
        err_doc, 
        "</body>\n"
        "</html>"
    );
    strcpy(result, err_doc);
}
