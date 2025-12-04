#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "http.h"

static struct string *get_status_string(enum request_status status)
{
    size_t version_size = strlen(HTTP_VERSION);

    switch (status)
    {
    case OK:
        return string_create(HTTP_VERSION " 200 OK",
                             strlen(" 200 OK") + version_size);
    case BAD_REQUEST:
        return string_create(HTTP_VERSION " 400 Bad Request",
                             strlen(" 400 Bad Request") + version_size);
    case FORBIDDEN:
        return string_create(HTTP_VERSION " 403 Forbidden",
                             strlen(" 403 Forbidden") + version_size);
    case NOT_FOUND:
        return string_create(HTTP_VERSION " 404 Not Found",
                             strlen(" 404 Not Found") + version_size);
    case METHOD_NOT_ALLOWED:
        return string_create(HTTP_VERSION " 405 Method Not Allowed",
                             strlen(" 405 Method Not Allowed") + version_size);
    case UNSUPPORTED_VERSION:
        return string_create(HTTP_VERSION " 505 HTTP Version Not Supported",
                             strlen(" 505 HTTP Version Not Supported")
                                 + version_size);
    default:
        return string_create(HTTP_VERSION " 500 Internal Server Error",
                             strlen(" 500 Internal Server Error")
                                 + version_size);
    }
}

struct response_header *create_response(const struct request_header *request,
                                        off_t content_length)
{
    struct response_header *response = malloc(sizeof(struct response_header));
    response->status = get_status_string(request->status);

    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    // Format time as Day (3 letters), DD Mon YYYY HH:MM:SS GMT
    char time_str[30];
    strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
    response->date = string_create(time_str, strlen(time_str));

    response->content_length = content_length;
    response->status_code = request->status;
    return response;
}

struct string *response_header_to_string(const struct response_header *response)
{
    struct string *header = string_create("", 0);

    char *field_end = "\r\n";
    size_t field_end_len = strlen(field_end);

    // Status line
    string_concat_str(header, response->status->data, response->status->size);
    string_concat_str(header, field_end, field_end_len);
    //  Date line
    string_concat_str(header, "Date: ", strlen("Date: "));
    string_concat_str(header, response->date->data, response->date->size);
    string_concat_str(header, field_end, field_end_len);

    if (response->status_code == METHOD_NOT_ALLOWED)
    {
        // Allow line for 405 Method Not Allowed
        string_concat_str(header, "Allow: GET, HEAD\r\n",
                          strlen("Allow: GET, HEAD\r\n"));
    }

    // Content-Length line
    char content_length_str[50];
    sprintf(content_length_str, "Content-Length: %ld\r\n",
            response->content_length);
    string_concat_str(header, content_length_str, strlen(content_length_str));

    // Connection line
    string_concat_str(header, "Connection: close\r\n",
                      strlen("Connection: close\r\n"));

    // End header
    string_concat_str(header, field_end, field_end_len);

    return header;
}

void destroy_response(struct response_header *response)
{
    if (!response)
        return;

    string_destroy(response->status);
    string_destroy(response->date);
    free(response);
}
