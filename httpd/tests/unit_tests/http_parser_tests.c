#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <string.h>

#include "../../src/http/http.h"
#include "../../src/utils/string/string.h"

static struct string *make_request(const char *s)
{
    size_t len = strlen(s);
    struct string *str = string_create(s, len);
    return str;
}

Test(http_parser, parse_valid_get_request)
{
    const char *req = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
    struct string *r = make_request(req);
    struct request_header *hdr = parse_request(r);

    cr_expect_not_null(hdr, "parse_request returned NULL");
    if (hdr)
    {
        cr_expect_eq(hdr->status, OK, "expected OK status");
        cr_expect_eq(hdr->method, GET, "expected GET method");
        cr_expect_not_null(hdr->filename, "filename not set");
        if (hdr->filename)
            cr_expect_eq(0,
                         strncmp(hdr->filename->data, "/index.html",
                                 strlen("/index.html")));
        cr_expect_not_null(hdr->host, "host not set");
        if (hdr->host)
            cr_expect_eq(
                0,
                strncmp(hdr->host->data, "example.com", strlen("example.com")));

        destroy_request(hdr);
    }
    string_destroy(r);
}

Test(http_parser, parse_valid_head_request)
{
    const char *req = "HEAD /foo HTTP/1.1\r\nHost: a\r\n\r\n";
    struct string *r = make_request(req);
    struct request_header *hdr = parse_request(r);

    cr_expect_not_null(hdr);
    if (hdr)
    {
        cr_expect_eq(hdr->status, OK);
        cr_expect_eq(hdr->method, HEAD);
        cr_expect_not_null(hdr->filename);
        if (hdr->filename)
            cr_expect_eq(0,
                         strncmp(hdr->filename->data, "/foo", strlen("/foo")));
        cr_expect_not_null(hdr->host);
        if (hdr->host)
            cr_expect_eq(0, strncmp(hdr->host->data, "a", strlen("a")));

        destroy_request(hdr);
    }
    string_destroy(r);
}

Test(http_parser, malformed_header_name)
{
    const char *req = "GET / HTTP/1.1\r\nBad!Name: value\r\n\r\n";
    struct string *r = make_request(req);
    struct request_header *hdr = parse_request(r);

    cr_expect_not_null(hdr);
    if (hdr)
    {
        cr_expect(hdr->status == OK || hdr->status == BAD_REQUEST);
        destroy_request(hdr);
    }
    string_destroy(r);
}

Test(http_parser, missing_host_header)
{
    const char *req = "GET / HTTP/1.1\r\n\r\n";
    struct string *r = make_request(req);
    struct request_header *hdr = parse_request(r);

    cr_expect_not_null(hdr);
    if (hdr)
    {
        if (hdr->status == OK)
            cr_expect_null(hdr->host);
        destroy_request(hdr);
    }
    string_destroy(r);
}
