#include "utils/http_server.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define READ_BUFFER_SIZE 4096

typedef struct
{
    char *data;
    size_t size;
    size_t capacity;
} Buffer;

typedef struct
{
    HttpServerHeader *items;
    size_t count;
    size_t capacity;
} HeaderList;

static void buffer_init(Buffer *buffer)
{
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}

static void buffer_free(Buffer *buffer)
{
    if (!buffer)
        return;
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}

static bool buffer_reserve(Buffer *buffer, size_t additional)
{
    size_t needed = buffer->size + additional + 1;
    if (needed <= buffer->capacity)
        return true;
    size_t new_cap = buffer->capacity == 0 ? 4096 : buffer->capacity;
    while (new_cap < needed)
        new_cap *= 2;
    char *resized = realloc(buffer->data, new_cap);
    if (!resized)
        return false;
    buffer->data = resized;
    buffer->capacity = new_cap;
    return true;
}

static bool buffer_append(Buffer *buffer, const char *data, size_t length)
{
    if (!buffer_reserve(buffer, length))
        return false;
    memcpy(buffer->data + buffer->size, data, length);
    buffer->size += length;
    buffer->data[buffer->size] = '\0';
    return true;
}

static void header_list_init(HeaderList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void header_list_free(HeaderList *list)
{
    if (!list)
        return;
    for (size_t i = 0; i < list->count; ++i)
    {
        free(list->items[i].name);
        free(list->items[i].value);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static bool header_list_reserve(HeaderList *list, size_t needed)
{
    if (needed <= list->capacity)
        return true;
    size_t new_cap = list->capacity == 0 ? 8 : list->capacity * 2;
    while (new_cap < needed)
        new_cap *= 2;
    HttpServerHeader *resized = realloc(list->items, new_cap * sizeof(HttpServerHeader));
    if (!resized)
        return false;
    list->items = resized;
    list->capacity = new_cap;
    return true;
}

static bool header_list_append(HeaderList *list, const char *name, const char *value)
{
    if (!header_list_reserve(list, list->count + 1))
        return false;
    char *name_copy = strdup(name ? name : "");
    char *value_copy = strdup(value ? value : "");
    if (!name_copy || !value_copy)
    {
        free(name_copy);
        free(value_copy);
        return false;
    }
    list->items[list->count].name = name_copy;
    list->items[list->count].value = value_copy;
    list->count++;
    return true;
}

static char *strndup_safe(const char *src, size_t len)
{
    char *copy = malloc(len + 1);
    if (!copy)
        return NULL;
    memcpy(copy, src, len);
    copy[len] = '\0';
    return copy;
}

static char *trim_whitespace(char *start, char *end)
{
    while (start < end && isspace((unsigned char)*start))
        start++;
    while (end > start && isspace((unsigned char)*(end - 1)))
        end--;
    size_t len = (size_t)(end - start);
    char *trimmed = malloc(len + 1);
    if (!trimmed)
        return NULL;
    memcpy(trimmed, start, len);
    trimmed[len] = '\0';
    return trimmed;
}

static const char *default_reason_phrase(int status)
{
    switch (status)
    {
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 202:
        return "Accepted";
    case 204:
        return "No Content";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 304:
        return "Not Modified";
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 409:
        return "Conflict";
    case 415:
        return "Unsupported Media Type";
    case 500:
        return "Internal Server Error";
    case 502:
        return "Bad Gateway";
    case 503:
        return "Service Unavailable";
    default:
        return "OK";
    }
}

static bool read_request_into_buffer(int client_fd, Buffer *buffer, size_t *header_length, size_t *body_length)
{
    *header_length = 0;
    *body_length = 0;
    bool headers_parsed = false;
    size_t expected_body = 0;

    while (true)
    {
        char chunk[READ_BUFFER_SIZE];
        ssize_t bytes = recv(client_fd, chunk, sizeof(chunk), 0);
        if (bytes == 0)
            break;
        if (bytes < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        if (!buffer_append(buffer, chunk, (size_t)bytes))
            return false;

        if (!headers_parsed)
        {
            char *header_end = strstr(buffer->data, "\r\n\r\n");
            if (header_end)
            {
                headers_parsed = true;
                *header_length = (size_t)(header_end - buffer->data) + 4;
                const char *cursor = buffer->data;
                const char *limit = buffer->data + *header_length;
                while (cursor < limit)
                {
                    const char *line_end = strstr(cursor, "\r\n");
                    if (!line_end)
                        break;
                    size_t line_len = (size_t)(line_end - cursor);
                    const char *colon = memchr(cursor, ':', line_len);
                    if (colon)
                    {
                        size_t name_len = (size_t)(colon - cursor);
                        if (name_len == 14 && strncasecmp(cursor, "Content-Length", 14) == 0)
                        {
                            const char *value_start = colon + 1;
                            while (value_start < line_end && isspace((unsigned char)*value_start))
                                value_start++;
                            char *value = strndup_safe(value_start, (size_t)(line_end - value_start));
                            if (!value)
                                return false;
                            expected_body = (size_t)strtoul(value, NULL, 10);
                            free(value);
                            break;
                        }
                    }
                    cursor = line_end + 2;
                }
            }
        }

        if (headers_parsed)
        {
            if (buffer->size >= *header_length + expected_body)
            {
                *body_length = buffer->size - *header_length;
                return true;
            }
        }
    }

    if (!headers_parsed)
        return false;
    *body_length = buffer->size - *header_length;
    return true;
}

static bool parse_request(Buffer *buffer, size_t header_length, HttpServerRequest *request)
{
    memset(request, 0, sizeof(*request));

    HeaderList headers;
    header_list_init(&headers);
    char *full_path = NULL;
    bool success = false;

    char *request_line_end = strstr(buffer->data, "\r\n");
    if (!request_line_end)
        goto cleanup;

    char *method_end = memchr(buffer->data, ' ', (size_t)(request_line_end - buffer->data));
    if (!method_end)
        goto cleanup;

    char *path_start = method_end + 1;
    char *path_end = memchr(path_start, ' ', (size_t)(request_line_end - path_start));
    if (!path_end)
        goto cleanup;

    request->method = strndup_safe(buffer->data, (size_t)(method_end - buffer->data));
    if (!request->method)
        goto cleanup;
    for (char *c = request->method; *c; ++c)
        *c = (char)toupper((unsigned char)*c);

    full_path = strndup_safe(path_start, (size_t)(path_end - path_start));
    if (!full_path)
        goto cleanup;

    char *query_start = strchr(full_path, '?');
    if (query_start)
    {
        *query_start = '\0';
        query_start++;
        request->query = strdup(query_start);
        if (!request->query)
            goto cleanup;
    }
    request->path = strdup(full_path);
    if (!request->path)
        goto cleanup;

    request->http_version = strndup_safe(path_end + 1, (size_t)(request_line_end - (path_end + 1)));
    if (!request->http_version)
        goto cleanup;

    {
        char *cursor = request_line_end + 2;
        char *headers_end = buffer->data + header_length;
        while (cursor < headers_end - 2)
        {
            char *line_end = strstr(cursor, "\r\n");
            if (!line_end)
                break;
            if (line_end == cursor)
            {
                cursor += 2;
                continue;
            }
            char *colon = memchr(cursor, ':', (size_t)(line_end - cursor));
            if (!colon)
                goto cleanup;
            char *name = strndup_safe(cursor, (size_t)(colon - cursor));
            if (!name)
                goto cleanup;
            for (char *c = name; *c; ++c)
                *c = (char)tolower((unsigned char)*c);

            char *value = trim_whitespace(colon + 1, line_end);
            if (!value)
            {
                free(name);
                goto cleanup;
            }
            bool appended = header_list_append(&headers, name, value);
            free(name);
            free(value);
            if (!appended)
                goto cleanup;
            cursor = line_end + 2;
        }
    }

    request->headers = headers.items;
    request->header_count = headers.count;
    headers.items = NULL;
    headers.count = 0;

    {
        size_t body_len = buffer->size - header_length;
        if (body_len > 0)
        {
            request->body = malloc(body_len + 1);
            if (!request->body)
                goto cleanup;
            memcpy(request->body, buffer->data + header_length, body_len);
            request->body[body_len] = '\0';
            request->body_length = body_len;
        }
    }

    success = true;

cleanup:
    free(full_path);
    if (!success)
    {
        header_list_free(&headers);
        http_server_request_cleanup(request);
    }
    return success;
}

static void free_request(HttpServerRequest *request)
{
    http_server_request_cleanup(request);
}

static bool response_has_header(const HttpServerResponse *response, const char *name)
{
    for (size_t i = 0; i < response->header_count; ++i)
    {
        if (strcasecmp(response->headers[i].name, name) == 0)
            return true;
    }
    return false;
}

static bool write_response(int client_fd, const HttpServerResponse *response)
{
    Buffer buffer;
    buffer_init(&buffer);

    const char *status_text = response->status_text ? response->status_text : default_reason_phrase(response->status_code);
    char status_line[256];
    int written = snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n", response->status_code, status_text ? status_text : "OK");
    if (written < 0 || !buffer_append(&buffer, status_line, (size_t)written))
    {
        buffer_free(&buffer);
        return false;
    }

    bool has_content_length = response_has_header(response, "Content-Length");
    bool has_connection = response_has_header(response, "Connection");

    for (size_t i = 0; i < response->header_count; ++i)
    {
        if (!response->headers[i].name)
            continue;
        size_t name_len = strlen(response->headers[i].name);
        size_t value_len = response->headers[i].value ? strlen(response->headers[i].value) : 0;
        if (!buffer_append(&buffer, response->headers[i].name, name_len))
        {
            buffer_free(&buffer);
            return false;
        }
        if (!buffer_append(&buffer, ": ", 2))
        {
            buffer_free(&buffer);
            return false;
        }
        if (!buffer_append(&buffer, response->headers[i].value ? response->headers[i].value : "", value_len))
        {
            buffer_free(&buffer);
            return false;
        }
        if (!buffer_append(&buffer, "\r\n", 2))
        {
            buffer_free(&buffer);
            return false;
        }
    }

    char content_length_header[64];
    if (!has_content_length)
    {
        size_t body_len = response->body ? response->body_length : 0;
        int len_written = snprintf(content_length_header, sizeof(content_length_header), "Content-Length: %zu\r\n", body_len);
        if (len_written < 0 || !buffer_append(&buffer, content_length_header, (size_t)len_written))
        {
            buffer_free(&buffer);
            return false;
        }
    }

    if (!has_connection)
    {
        if (!buffer_append(&buffer, "Connection: close\r\n", strlen("Connection: close\r\n")))
        {
            buffer_free(&buffer);
            return false;
        }
    }

    if (!buffer_append(&buffer, "\r\n", 2))
    {
        buffer_free(&buffer);
        return false;
    }

    if (response->body && response->body_length > 0)
    {
        if (!buffer_append(&buffer, response->body, response->body_length))
        {
            buffer_free(&buffer);
            return false;
        }
    }

    size_t total = buffer.size;
    size_t sent = 0;
    while (sent < total)
    {
        ssize_t n = send(client_fd, buffer.data + sent, total - sent, 0);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            buffer_free(&buffer);
            return false;
        }
        sent += (size_t)n;
    }

    buffer_free(&buffer);
    return true;
}

void http_server_request_cleanup(HttpServerRequest *request)
{
    if (!request)
        return;
    free(request->method);
    free(request->path);
    free(request->query);
    free(request->http_version);
    for (size_t i = 0; i < request->header_count; ++i)
    {
        free(request->headers[i].name);
        free(request->headers[i].value);
    }
    free(request->headers);
    free(request->body);
    memset(request, 0, sizeof(*request));
}

void http_server_response_init(HttpServerResponse *response)
{
    if (!response)
        return;
    response->status_code = 200;
    response->status_text = NULL;
    response->headers = NULL;
    response->header_count = 0;
    response->body = NULL;
    response->body_length = 0;
}

void http_server_response_cleanup(HttpServerResponse *response)
{
    if (!response)
        return;
    free(response->status_text);
    for (size_t i = 0; i < response->header_count; ++i)
    {
        free(response->headers[i].name);
        free(response->headers[i].value);
    }
    free(response->headers);
    free(response->body);
    memset(response, 0, sizeof(*response));
}

bool http_server_response_set_status(HttpServerResponse *response, int status_code, const char *status_text)
{
    if (!response)
        return false;
    response->status_code = status_code;
    free(response->status_text);
    response->status_text = status_text ? strdup(status_text) : NULL;
    return status_text == NULL || response->status_text != NULL;
}

bool http_server_response_set_body(HttpServerResponse *response, const char *body, size_t length)
{
    if (!response)
        return false;
    free(response->body);
    if (!body)
    {
        response->body = NULL;
        response->body_length = 0;
        return true;
    }
    response->body = malloc(length + 1);
    if (!response->body)
        return false;
    memcpy(response->body, body, length);
    response->body[length] = '\0';
    response->body_length = length;
    return true;
}

bool http_server_response_add_header(HttpServerResponse *response, const char *name, const char *value)
{
    if (!response || !name)
        return false;
    HttpServerHeader *resized = realloc(response->headers, sizeof(HttpServerHeader) * (response->header_count + 1));
    if (!resized)
        return false;
    response->headers = resized;
    char *name_copy = strdup(name);
    char *value_copy = strdup(value ? value : "");
    if (!name_copy || !value_copy)
    {
        free(name_copy);
        free(value_copy);
        return false;
    }
    response->headers[response->header_count].name = name_copy;
    response->headers[response->header_count].value = value_copy;
    response->header_count++;
    return true;
}

bool http_server_listen(const char *host,
                        const char *port,
                        HttpServerHandler handler,
                        void *user_data,
                        char **error_message)
{
    if (error_message)
        *error_message = NULL;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *result = NULL;
    int ret = getaddrinfo(host, port, &hints, &result);
    if (ret != 0)
    {
        if (error_message)
            *error_message = strdup(gai_strerror(ret));
        return false;
    }

    int listen_fd = -1;
    struct addrinfo *rp = NULL;
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd == -1)
            continue;

        int optval = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;

        close(listen_fd);
        listen_fd = -1;
    }

    freeaddrinfo(result);

    if (listen_fd == -1)
    {
        if (error_message)
            *error_message = strdup("Failed to bind socket");
        return false;
    }

    if (listen(listen_fd, 16) == -1)
    {
        if (error_message)
            *error_message = strdup(strerror(errno));
        close(listen_fd);
        return false;
    }

    bool continue_running = true;
    while (continue_running)
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0)
        {
            if (errno == EINTR)
                continue;
            if (error_message)
                *error_message = strdup(strerror(errno));
            close(listen_fd);
            return false;
        }

        Buffer buffer;
        buffer_init(&buffer);
        size_t header_length = 0;
        size_t body_length = 0;
        bool read_ok = read_request_into_buffer(client_fd, &buffer, &header_length, &body_length);
        HttpServerRequest request;
        HttpServerResponse response;
        http_server_response_init(&response);
        bool parsed = false;

        if (read_ok)
            parsed = parse_request(&buffer, header_length, &request);

        if (!read_ok || !parsed)
        {
            http_server_response_set_status(&response, 400, "Bad Request");
            http_server_response_set_body(&response, "Bad Request", strlen("Bad Request"));
            write_response(client_fd, &response);
        }
        else
        {
            continue_running = handler ? handler(&request, &response, user_data) : false;
            write_response(client_fd, &response);
            free_request(&request);
        }

        http_server_response_cleanup(&response);
        buffer_free(&buffer);
        close(client_fd);
    }

    close(listen_fd);
    return true;
}
