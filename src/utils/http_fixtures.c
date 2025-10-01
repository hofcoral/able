#include "utils/http_fixtures.h"

#include <stdlib.h>
#include <string.h>

typedef struct
{
    const char *name;
    const char *value;
} FixtureHeader;

typedef struct
{
    const char *method;
    const char *url;
    long status_code;
    const char *status_text;
    const char *final_url;
    const char *body;
    const FixtureHeader *headers;
    size_t header_count;
} HttpFixture;

static const FixtureHeader GET_HEADERS[] = {
    {"Content-Type", "application/json"},
    {"X-Able-Fixture", "network"},
};

static const FixtureHeader POST_HEADERS[] = {
    {"Content-Type", "application/json"},
    {"X-Able-Fixture", "network"},
};

static const FixtureHeader AUTH_HEADERS[] = {
    {"Content-Type", "application/json"},
    {"X-Able-Fixture", "network"},
};

static const HttpFixture FIXTURES[] = {
    {
        .method = "GET",
        .url = "https://httpbin.org/get",
        .status_code = 200,
        .status_text = "OK",
        .final_url = "https://httpbin.org/get",
        .body = "{\"args\":{},\"headers\":{\"Accept\":\"*/*\",\"Host\":\"httpbin.org\"}}",
        .headers = GET_HEADERS,
        .header_count = sizeof(GET_HEADERS) / sizeof(GET_HEADERS[0]),
    },
    {
        .method = "POST",
        .url = "https://httpbin.org/post",
        .status_code = 200,
        .status_text = "OK",
        .final_url = "https://httpbin.org/post",
        .body = "{\"data\":\"Hello World\",\"json\":null,\"headers\":{\"Content-Type\":\"text/plain\"}}",
        .headers = POST_HEADERS,
        .header_count = sizeof(POST_HEADERS) / sizeof(POST_HEADERS[0]),
    },
    {
        .method = "GET",
        .url = "https://httpbin.org/basic-auth/user/passwd",
        .status_code = 200,
        .status_text = "OK",
        .final_url = "https://httpbin.org/basic-auth/user/passwd",
        .body = "{\"authenticated\":true,\"user\":\"user\"}",
        .headers = AUTH_HEADERS,
        .header_count = sizeof(AUTH_HEADERS) / sizeof(AUTH_HEADERS[0]),
    },
};

static bool fixtures_equal(const char *expected, const char *actual)
{
    if (!expected || !actual)
        return false;
    return strcmp(expected, actual) == 0;
}

static void cleanup_response_partial(HttpResponse *response, size_t allocated_headers)
{
    if (!response)
        return;
    free(response->status_text);
    response->status_text = NULL;
    free(response->final_url);
    response->final_url = NULL;
    free(response->body);
    response->body = NULL;
    if (response->headers)
    {
        for (size_t i = 0; i < allocated_headers; ++i)
        {
            free(response->headers[i].name);
            free(response->headers[i].value);
        }
        free(response->headers);
        response->headers = NULL;
    }
    response->header_count = 0;
}

static bool populate_response(const HttpFixture *fixture, HttpResponse *response)
{
    response->status_code = fixture->status_code;
    response->status_text = strdup(fixture->status_text ? fixture->status_text : "");
    if (!response->status_text)
        return false;

    response->final_url = strdup(fixture->final_url ? fixture->final_url : "");
    if (!response->final_url)
    {
        cleanup_response_partial(response, 0);
        return false;
    }

    response->body = strdup(fixture->body ? fixture->body : "");
    if (!response->body)
    {
        cleanup_response_partial(response, 0);
        return false;
    }

    if (fixture->header_count == 0)
        return true;

    response->headers = calloc(fixture->header_count, sizeof(HttpResponseHeader));
    if (!response->headers)
    {
        cleanup_response_partial(response, 0);
        return false;
    }

    for (size_t i = 0; i < fixture->header_count; ++i)
    {
        response->headers[i].name = strdup(fixture->headers[i].name ? fixture->headers[i].name : "");
        if (!response->headers[i].name)
        {
            cleanup_response_partial(response, i);
            return false;
        }

        response->headers[i].value = strdup(fixture->headers[i].value ? fixture->headers[i].value : "");
        if (!response->headers[i].value)
        {
            cleanup_response_partial(response, i + 1);
            return false;
        }
    }

    response->header_count = fixture->header_count;
    return true;
}

bool http_fixtures_try_get(const char *method, const char *url, HttpResponse *response)
{
    if (!method || !url || !response)
        return false;

    for (size_t i = 0; i < sizeof(FIXTURES) / sizeof(FIXTURES[0]); ++i)
    {
        const HttpFixture *fixture = &FIXTURES[i];
        if (!fixtures_equal(fixture->method, method) || !fixtures_equal(fixture->url, url))
            continue;

        if (!populate_response(fixture, response))
        {
            cleanup_response_partial(response, response->header_count);
            return false;
        }

        return true;
    }

    return false;
}
