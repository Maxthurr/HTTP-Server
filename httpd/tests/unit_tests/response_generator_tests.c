#include <criterion/criterion.h>
#include <string.h>

#include "../../src/http/http.h"
#include "../../src/utils/string/string.h"

static bool contains_substr(const char *buf, size_t n, const char *needle)
{
    size_t m = strlen(needle);
    if (m == 0 || n < m)
        return false;
    for (size_t i = 0; i + m <= n; ++i)
    {
        if (memcmp(buf + i, needle, m) == 0)
            return true;
    }
    return false;
}

TestSuite(response_generator);

Test(response_generator, ok_response)
{
    struct request_header request = { 0 };
    request.status = OK;

    struct response_header *res = create_response(&request, 12345);
    cr_assert_not_null(res, "Response header should not be NULL");
    cr_assert_not_null(res->status, "Status string should not be NULL");
    cr_assert_not_null(res->date, "Date string should not be NULL");

    const char *expected = HTTP_VERSION " 200 OK";
    size_t expected_len = strlen(HTTP_VERSION) + strlen(" 200 OK");

    cr_expect_eq(res->status->size, expected_len,
                 "Expected status size %zu, got %zu", expected_len,
                 res->status->size);
    cr_expect(memcmp(res->status->data, expected, expected_len) == 0,
              "Status strings do not match");

    cr_expect_neq(res->date->size, 0, "Date string size should not be zero");
    cr_expect(contains_substr(res->date->data, res->date->size, "GMT"),
              "Date string should contain 'GMT'");

    cr_expect_eq(res->content_length, 12345,
                 "Expected content length 12345, got %ld", res->content_length);

    destroy_response(res);
}

Test(response_generator, bad_request)
{
    struct request_header request = { 0 };
    request.status = BAD_REQUEST;

    struct response_header *res = create_response(&request, 0);
    cr_assert_not_null(res, "Response header should not be NULL");
    const char *expected = HTTP_VERSION " 400 Bad Request";
    size_t expected_len = strlen(HTTP_VERSION) + strlen(" 400 Bad Request");

    cr_expect_eq(res->status->size, expected_len,
                 "Expected status size %zu, got %zu", expected_len,
                 res->status->size);
    cr_expect(memcmp(res->status->data, expected, expected_len) == 0,
              "Status strings do not match");

    destroy_response(res);
}

Test(response_generator, various_status_strings)
{
    struct request_header request = { 0 };

    request.status = FORBIDDEN;
    struct response_header *r1 = create_response(&request, 0);
    cr_expect_not_null(r1, "Response header should not be NULL");
    cr_expect(memcmp(r1->status->data, HTTP_VERSION " 403 Forbidden",
                     r1->status->size)
                  == 0,
              "Status strings do not match");
    destroy_response(r1);

    request.status = NOT_FOUND;
    struct response_header *r2 = create_response(&request, 0);
    cr_expect_not_null(r2, "Response header should not be NULL");
    cr_expect(memcmp(r2->status->data, HTTP_VERSION " 404 Not Found",
                     r2->status->size)
                  == 0,
              "Status strings do not match");
    destroy_response(r2);

    request.status = METHOD_NOT_ALLOWED;
    struct response_header *r3 = create_response(&request, 0);
    cr_expect_not_null(r3, "Response header should not be NULL");
    cr_expect(memcmp(r3->status->data, HTTP_VERSION " 405 Method Not Allowed",
                     r3->status->size)
                  == 0,
              "Status strings do not match");
    destroy_response(r3);

    request.status = UNSUPPORTED_VERSION;
    struct response_header *r4 = create_response(&request, 0);
    cr_expect_not_null(r4, "Response header should not be NULL");
    cr_expect(memcmp(r4->status->data,
                     HTTP_VERSION " 505 HTTP Version Not Supported",
                     r4->status->size)
                  == 0,
              "Status strings do not match");
    destroy_response(r4);
}

Test(response_generator, default_internal_server_error)
{
    struct request_header request = { 0 };
    request.status = 0;
    struct response_header *r = create_response(&request, 0);
    cr_expect_not_null(r, "Response header should not be NULL");
    cr_expect(memcmp(r->status->data, HTTP_VERSION " 500 Internal Server Error",
                     r->status->size)
                  == 0,
              "Status strings do not match");
    destroy_response(r);
}

Test(response_generator, date_contains_gmt_and_nonzero_length)
{
    struct request_header request = { 0 };
    request.status = OK;
    struct response_header *r = create_response(&request, 10);
    cr_expect_not_null(r, "Response header should not be NULL");
    cr_expect(r->date->size > 0, "Date string size should not be zero");
    cr_expect(contains_substr(r->date->data, r->date->size, "GMT"),
              "Date string should contain 'GMT'");
    destroy_response(r);
}
