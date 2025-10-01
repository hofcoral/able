#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    char *name;
    char *value;
} HttpServerHeader;

typedef struct
{
    char *method;
    char *path;
    char *query;
    char *http_version;
    HttpServerHeader *headers;
    size_t header_count;
    char *body;
    size_t body_length;
} HttpServerRequest;

typedef struct
{
    int status_code;
    char *status_text;
    HttpServerHeader *headers;
    size_t header_count;
    char *body;
    size_t body_length;
} HttpServerResponse;

typedef bool (*HttpServerHandler)(const HttpServerRequest *request,
                                  HttpServerResponse *response,
                                  void *user_data);

void http_server_request_cleanup(HttpServerRequest *request);

void http_server_response_init(HttpServerResponse *response);
void http_server_response_cleanup(HttpServerResponse *response);
bool http_server_response_set_status(HttpServerResponse *response, int status_code, const char *status_text);
bool http_server_response_set_body(HttpServerResponse *response, const char *body, size_t length);
bool http_server_response_add_header(HttpServerResponse *response, const char *name, const char *value);

bool http_server_listen(const char *host,
                        const char *port,
                        HttpServerHandler handler,
                        void *user_data,
                        char **error_message);

#ifdef __cplusplus
}
#endif

#endif
