#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../config/config.h"
#include "../utils/string/string.h"
#include "http.h"

static enum http_method get_method(const char *data, size_t size)
{
    if (size >= 3 && !memcmp(data, "GET", 3))
        return GET;
    if (size >= 4 && !memcmp(data, "HEAD", 4))
        return HEAD;

    return UNKNOWN;
}

static size_t get_filename_length(const char *data, size_t size, size_t start)
{
    size_t i = start;

    // Traverse file name
    while (i < size && data[i] != ' ')
        i++;

    if (i == size)
        return 0;

    return i - start;
}

static bool end_of_header(struct string *request, size_t index)
{
    return request->data[index] == '\r' && request->data[index + 1] == '\n';
}

static bool is_valid_field_name_char(char c)
{
    if (isalnum(c))
        return true;

    switch (c)
    {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
        return true;
    default:
        return false;
    }
}

static bool is_valid_header_line(struct string *request, size_t index)
{
    char *data = request->data;
    // A valid header line must contain a colon ':'
    while (index + 1 < request->size
           && !(data[index] == '\r' && data[index + 1] == '\n')
           && is_valid_field_name_char(data[index]))
    {
        index++;
    }

    return index + 1 < request->size && data[index] == ':';
}

static bool parse_host_field(struct string *request, size_t *index,
                             struct request_header *req_header)
{
    char *data = request->data;
    size_t i = *index + 5; // Skip field name "Host:"
    size_t host_length = 0;

    // Skip spaces after colon
    while (i < request->size && (data[i] == ' ' || data[i] == '\t'))
        i++;

    size_t start = i;
    // Get length of host field value
    while (i < request->size
           && (is_valid_field_name_char(request->data[i]) || data[i] == ':')
           && (data[i] != ' ' || data[i] != '\t'))
        i++;

    host_length = i - start;

    // Skip spaces after colon
    while (i < request->size && (data[i] == ' ' || data[i] == '\t'))
        i++;

    // Move to the end of the line
    while (i + 1 < request->size && !(data[i] == '\r' && data[i + 1] == '\n'))
        i++;

    // Unfinished header
    if (i >= request->size)
        return false;

    req_header->host = string_create(data + start, host_length);
    return true;
}

static void parse_headers(struct string *request, size_t i,
                          struct request_header *req_header)
{
    char *data = request->data;

    // Traverse header fields
    while (!end_of_header(request, i))
    {
        // Host field line
        if (i + 5 < request->size && string_n_casecmp(data + i, "Host:", 5))
        {
            if (req_header->host || !parse_host_field(request, &i, req_header))
            {
                req_header->status = BAD_REQUEST;
                return;
            }
        }
        // Check validity of other field lines
        else if (!is_valid_header_line(request, i))
        {
            req_header->status = BAD_REQUEST;
            return;
        }

        // Move to next line
        while (i + 2 < request->size
               && !(data[i] == '\r' && data[i + 1] == '\n'))
            i++;

        i += 2; // Skip CRLF
    }
}

static void parse_filename(struct string *request, size_t *i,
                           struct request_header *req_header,
                           const struct config *config)
{
    size_t filename_len = get_filename_length(request->data, request->size, *i);
    if (filename_len == 0)
        req_header->status = BAD_REQUEST;

    req_header->target = string_create(request->data + *i, filename_len);
    req_header->filename = string_create(config->servers->root_dir,
                                         strlen(config->servers->root_dir));
    string_concat_str(req_header->filename, request->data + *i, filename_len);

    // If given target is a directory, append default file
    if (req_header->filename->data[req_header->filename->size - 1] == '/')
    {
        string_concat_str(req_header->filename, config->servers->default_file,
                          strlen(config->servers->default_file));
    }

    // Add null byte
    string_concat_str(req_header->filename, "", 1);
    *i += filename_len;
}

static void parse_version(struct string *request, size_t *i,
                          struct request_header *req_header)
{
    // Traverse HTTP version

    req_header->version = string_create(request->data + *i, 8); // "HTTP/x.x"
    if (memcmp(req_header->version->data, HTTP_VERSION, 8))
        req_header->status = UNSUPPORTED_VERSION;

    *i += req_header->version->size;
}

static size_t parse_start(struct string *request,
                          struct request_header *req_header,
                          const struct config *config)
{
    size_t i = 0;
    req_header->method = get_method(request->data, request->size);
    if (req_header->method == UNKNOWN)
        req_header->status = METHOD_NOT_ALLOWED;

    // Move to the end of method string
    while (i < request->size && request->data[i] != ' ')
        i++;

    // Request line malformed
    if (i + 2 >= request->size || request->data[i++] != ' '
        || (request->data[i] == '\r' && request->data[i + 1] == '\n'))
        req_header->status = BAD_REQUEST;

    parse_filename(request, &i, req_header, config);

    // Request line malformed
    if (i + 2 >= request->size || request->data[i++] != ' '
        || (request->data[i] == '\r' && request->data[i + 1] == '\n'))
        req_header->status = BAD_REQUEST;

    parse_version(request, &i, req_header);

    // Wrongly terminated request line
    if (i + 2 >= request->size || request->data[i] != '\r'
        || request->data[i + 1] != '\n')
        req_header->status = BAD_REQUEST;

    i += 2;

    return i;
}

static bool is_valid_host(const struct config *config, struct string *host)
{
    // Basic validation: check if host is not empty
    if (host->size == 0)
        return false;

    // If host matches server_name
    if (host->size == config->servers->server_name->size
        && memcmp(host->data, config->servers->server_name->data, host->size)
            == 0)
        return true;

    size_t i = 0;
    // Get IP address
    while (i < host->size && host->data[i] != ':')
        i++;

    // Compare IP address
    size_t cfg_ip_len = strlen(config->servers->ip);
    if (i == cfg_ip_len
        && memcmp(host->data, config->servers->ip, cfg_ip_len) == 0)
    {
        const char *host_port_str;
        size_t host_port_len;

        if (i == host->size)
            return true;

        if (i + 1 == host->size) // No port specified, use default port 80 as
                                 // per RFC 9110
        {
            host_port_str = "80";
            host_port_len = 2;
        }
        else // Port specified in header
        {
            host_port_str = host->data + i + 1;
            host_port_len = host->size - i - 1;
        }

        size_t cfg_port_len = strlen(config->servers->port);
        if (host_port_len == cfg_port_len
            && memcmp(host_port_str, config->servers->port, cfg_port_len) == 0)
            return true;
    }

    return false;
}

struct request_header *parse_request(struct string *request,
                                     const struct config *config)
{
    if (!request || request->size == 0)
        return NULL;

    struct request_header *req_header =
        calloc(1, sizeof(struct request_header));

    req_header->status = OK;
    size_t i = parse_start(request, req_header, config);
    if (req_header->status != OK)
        return req_header;

    parse_headers(request, i, req_header);

    if (req_header->status != OK)
        return req_header;

    // Check mandatory Host header
    if (req_header->host == NULL || !is_valid_host(config, req_header->host))
        req_header->status = BAD_REQUEST;

    return req_header;
}

void destroy_request(struct request_header *request)
{
    if (!request)
        return;

    string_destroy(request->filename);
    string_destroy(request->version);
    string_destroy(request->target);
    string_destroy(request->host);
    free(request);
}
