#ifndef _ERR_DOC_H_
#define _ERR_DOC_H_

/**
 * HTTP status code that the proxy server currently supports
 */
enum HTTP_status_code {
    BAD_REQUEST,
    NOT_FOUND,
    INTERNAL_SERVER_ERROR,
    NOT_IMPLEMENTED,
    BAD_GATEWAY,
    SERVICE_UNAVAILABLE,
    NUM_HTTP_STATUS
};

/**
 * title of error document categorized by status code
 */
static const char* ERR_DOC_HEADING[NUM_HTTP_STATUS] = {
    "400 Bad Request",
    "404 Not Found",
    "500 Internal Server Error",
    "501 Not Implemented",
    "502 Bad Gateway",
    "503 Service Unavailable"
};

/**
 * error document description categorized by status code
 */
static const char* ERR_DOC_DESC[NUM_HTTP_STATUS] = {
    "<p>Received invalid request.</p>\n",
    "<p>Resource is not found on remote server.</p>\n",
    "<p>Internal error occurred in proxy server. Please refresh the webpage or try again later. If the problem persists, please report the issue to the webmaster.</p>\n",
    "<p>Unable to parse HTTP request.</p>\n",
    "<p>Received invalid response from remote server. Please refresh the webpage or try again later.</p>\n",
    "<p>Server is busy. Please refresh the webpage or try again later.</p>\n"
};

extern int map_status_code(const int status_code);
extern void gen_err_doc(const int status_code, const char* desc, char* result);

#endif
