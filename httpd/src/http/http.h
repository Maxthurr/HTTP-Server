#ifndef HTTP_H
#define HTTP_H

#include <sys/types.h>

#include "../config/config.h"
#include "../utils/string/string.h"

#define HTTP_VERSION "HTTP/1.1"

enum http_method
{
    GET,
    HEAD,
    UNKNOWN
};

enum request_status
{
    OK = 200,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    UNSUPPORTED_VERSION = 505
};

struct request_header
{
    enum http_method method;
    enum request_status status;
    struct string *filename;
    struct string *target;
    struct string *version;
    struct string *host;
};

struct response_header
{
    enum request_status status_code;
    struct string *status;
    struct string *date;
    off_t content_length;
};

// HTTP Request
struct request_header *parse_request(struct string *request,
                                     const struct config *config);
void destroy_request(struct request_header *request);

// HTTP Response
struct response_header *create_response(const struct request_header *request,
                                        off_t content_length);
struct string *
response_header_to_string(const struct response_header *response);
void destroy_response(struct response_header *response);

#endif /* ! HTTP_H */
