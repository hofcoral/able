#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    const char *name;
    const char *value;
} HttpRequestHeader;

typedef struct
{
    const char *body;
    const HttpRequestHeader *headers;
    size_t header_count;
    const char *cache_control;
    const char *credentials;
    const char *integrity;
    const char *refferer;
} HttpRequestOptions;

typedef struct
{
    char *name;
    char *value;
} HttpResponseHeader;

typedef struct
{
    long status_code;
    char *status_text;
    char *final_url;
    HttpResponseHeader *headers;
    size_t header_count;
    char *body;
} HttpResponse;

bool http_client_perform(const char *method,
                         const char *url,
                         const HttpRequestOptions *options,
                         HttpResponse *response,
                         char **error_message);

void http_response_cleanup(HttpResponse *response);

#endif
