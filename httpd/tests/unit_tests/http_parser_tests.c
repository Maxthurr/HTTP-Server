#define _POSIX_C_SOURCE 200809L

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/http/http.h"
#include "../../src/utils/string/string.h"

static struct string *make_request(const char *s)
{
    size_t len = strlen(s);
    struct string *str = string_create(s, len);
    return str;
}

static struct config *make_config_with_server_name(const char *name)
{
    struct config *config = calloc(1, sizeof(*config));
    config->servers = calloc(1, sizeof(*config->servers));
    size_t nlen = strlen(name);
    config->servers->server_name = string_create(name, nlen);
    config->servers->ip = strdup("127.0.0.1");
    config->servers->port = strdup("80");
    config->servers->root_dir = strdup("");
    config->servers->default_file = strdup("index.html");
    return config;
}

static struct config *make_config_with_ip_port(const char *ip, const char *port)
{
    struct config *config = calloc(1, sizeof(*config));
    config->servers = calloc(1, sizeof(*config->servers));
    config->servers->server_name = string_create("q", 1);
    config->servers->ip = strdup(ip);
    config->servers->port = strdup(port);
    config->servers->root_dir = strdup("");
    config->servers->default_file = strdup("index.html");
    return config;
}

TestSuite(http_parser);

Test(http_parser, malformed_status_line)
{
    const char *request = "G ET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_server_name("example.com");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, BAD_REQUEST);
        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, unsupported_http_version)
{
    const char *request = "GET / HTTP/2.0\r\nHost: example.com\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_server_name("example.com");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, UNSUPPORTED_VERSION);
        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, invalid_header_field_name_space)
{
    const char *request = "GET / HTTP/1.1\r\nBad Name: v\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_server_name("example.com");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, BAD_REQUEST);
        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, request_target_with_space)
{
    const char *request = "GET /foo bar HTTP/1.1\r\nHost: example.com\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_server_name("example.com");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, BAD_REQUEST);
        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, host_valid_ip_port)
{
    const char *request = "GET / HTTP/1.1\r\nHost: 127.0.0.1:8080\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_ip_port("127.0.0.1", "8080");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, OK);
        cr_expect_not_null(req_header->host);
        if (req_header->host)
            cr_expect_eq(0,
                         strncmp(req_header->host->data, "127.0.0.1:8080",
                                 strlen("127.0.0.1:8080")));

        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, valid_get_request)
{
    const char *request =
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_server_name("example.com");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header, "parse_request returned NULL");
    if (req_header)
    {
        cr_expect_eq(req_header->status, OK, "expected OK status but got %d",
                     req_header->status);
        cr_expect_eq(req_header->method, GET, "expected GET method");
        cr_expect_not_null(req_header->filename, "filename not set");
        if (req_header->filename)
            cr_expect_eq(0,
                         strncmp(req_header->filename->data, "/index.html",
                                 strlen("/index.html")));
        cr_expect_not_null(req_header->host, "host not set");
        if (req_header->host)
            cr_expect_eq(0,
                         strncmp(req_header->host->data, "example.com",
                                 strlen("example.com")));

        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, valid_head_request)
{
    const char *request = "HEAD /foo HTTP/1.1\r\nHost: a\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_server_name("a");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, OK);
        cr_expect_eq(req_header->method, HEAD);
        cr_expect_not_null(req_header->filename);
        if (req_header->filename)
            cr_expect_eq(
                0, strncmp(req_header->filename->data, "/foo", strlen("/foo")));
        cr_expect_not_null(req_header->host);
        if (req_header->host)
            cr_expect_eq(0, strncmp(req_header->host->data, "a", strlen("a")));

        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, malformed_header_name)
{
    const char *request = "GET / HTTP/1.1\r\nBad!Name: value\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_server_name("example.com");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect(req_header->status == OK
                  || req_header->status == BAD_REQUEST);
        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, missing_host_header)
{
    const char *request = "GET / HTTP/1.1\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_server_name("example.com");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, BAD_REQUEST);
        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, multiple_host_header)
{
    const char *request =
        "GET / HTTP/1.1\r\nHost: example.com\r\nHost: example.com\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_server_name("example.com");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, BAD_REQUEST);
        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, unfinished_ip_host)
{
    const char *request = "GET / HTTP/1.1\r\nHost: 127.0.0\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_ip_port("127.0.0.1", "80");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, BAD_REQUEST);
        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, ip_with_empty_port_host)
{
    const char *request = "GET / HTTP/1.1\r\nHost: 127.0.0.1:\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_ip_port("127.0.0.1", "880");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, BAD_REQUEST);
        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}

Test(http_parser, unspecified_port_server_port_80)
{
    const char *request = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    struct string *r = make_request(request);
    struct config *config = make_config_with_ip_port("127.0.0.1", "80");
    struct request_header *req_header = parse_request(r, config);

    cr_expect_not_null(req_header);
    if (req_header)
    {
        cr_expect_eq(req_header->status, OK);
        destroy_request(req_header);
        config_destroy(config);
    }
    string_destroy(r);
}
