#include "utils/http_client.h"

#include <ctype.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct
{
    char *data;
    size_t size;
} MemoryBuffer;

typedef struct
{
    HttpResponseHeader *headers;
    size_t count;
    size_t capacity;
    char *status_text;
} HeaderAccumulator;

static bool curl_initialized = false;

static void memory_buffer_init(MemoryBuffer *buffer)
{
    buffer->size = 0;
    buffer->data = malloc(1);
    if (buffer->data)
        buffer->data[0] = '\0';
}

static void memory_buffer_free(MemoryBuffer *buffer)
{
    if (!buffer)
        return;
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
}

static size_t write_body_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total_size = size * nmemb;
    MemoryBuffer *mem = (MemoryBuffer *)userp;
    char *ptr = realloc(mem->data, mem->size + total_size + 1);
    if (!ptr)
        return 0;
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, total_size);
    mem->size += total_size;
    mem->data[mem->size] = '\0';
    return total_size;
}

static void header_accumulator_init(HeaderAccumulator *acc)
{
    acc->headers = NULL;
    acc->count = 0;
    acc->capacity = 0;
    acc->status_text = NULL;
}

static void header_accumulator_free(HeaderAccumulator *acc)
{
    if (!acc)
        return;
    for (size_t i = 0; i < acc->count; ++i)
    {
        free(acc->headers[i].name);
        free(acc->headers[i].value);
    }
    free(acc->headers);
    acc->headers = NULL;
    acc->count = 0;
    acc->capacity = 0;
    free(acc->status_text);
    acc->status_text = NULL;
}

static char *trim_whitespace(char *str)
{
    if (!str)
        return str;
    while (*str && isspace((unsigned char)*str))
        ++str;
    if (!*str)
        return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        --end;
    end[1] = '\0';
    return str;
}

static bool header_accumulator_add(HeaderAccumulator *acc, const char *name, const char *value)
{
    if (acc->count == acc->capacity)
    {
        size_t new_cap = acc->capacity == 0 ? 4 : acc->capacity * 2;
        HttpResponseHeader *resized = realloc(acc->headers, new_cap * sizeof(HttpResponseHeader));
        if (!resized)
            return false;
        acc->headers = resized;
        acc->capacity = new_cap;
    }

    acc->headers[acc->count].name = strdup(name ? name : "");
    acc->headers[acc->count].value = strdup(value ? value : "");
    if (!acc->headers[acc->count].name || !acc->headers[acc->count].value)
        return false;
    acc->count++;
    return true;
}

static size_t write_header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    size_t total = size * nitems;
    HeaderAccumulator *acc = (HeaderAccumulator *)userdata;
    char *line = malloc(total + 1);
    if (!line)
        return 0;
    memcpy(line, buffer, total);
    line[total] = '\0';

    char *trimmed = trim_whitespace(line);
    if (trimmed[0] == '\0')
    {
        free(line);
        return total;
    }

    if (strncmp(trimmed, "HTTP/", 5) == 0)
    {
        const char *status = strchr(trimmed, ' ');
        if (status)
        {
            status = strchr(status + 1, ' ');
            if (status)
            {
                status = trim_whitespace((char *)(status + 1));
                free(acc->status_text);
                acc->status_text = strdup(status ? status : "");
            }
        }
        free(line);
        return total;
    }

    char *colon = strchr(trimmed, ':');
    if (colon)
    {
        *colon = '\0';
        char *name = trim_whitespace(trimmed);
        char *value = trim_whitespace(colon + 1);
        if (!header_accumulator_add(acc, name, value))
        {
            free(line);
            return 0;
        }
    }

    free(line);
    return total;
}

static bool ensure_curl_initialized(char **error_message)
{
    if (curl_initialized)
        return true;
    CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (code != CURLE_OK)
    {
        if (error_message)
        {
            const char *msg = curl_easy_strerror(code);
            *error_message = strdup(msg ? msg : "Failed to initialize curl");
        }
        return false;
    }
    curl_initialized = true;
    return true;
}

static struct curl_slist *append_header(struct curl_slist *list, const char *name, const char *value)
{
    size_t len = strlen(name) + 2 + strlen(value) + 1;
    char *header = malloc(len);
    if (!header)
        return NULL;
    snprintf(header, len, "%s: %s", name, value);
    struct curl_slist *updated = curl_slist_append(list, header);
    free(header);
    return updated;
}

static bool try_append_header(struct curl_slist **list, const char *name, const char *value)
{
    struct curl_slist *next = append_header(*list, name, value);
    if (!next)
        return false;
    *list = next;
    return true;
}

bool http_client_perform(const char *method,
                         const char *url,
                         const HttpRequestOptions *options,
                         HttpResponse *response,
                         char **error_message)
{
    if (!method || !url || !response)
        return false;
    if (!ensure_curl_initialized(error_message))
        return false;

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        if (error_message)
            *error_message = strdup("Unable to initialize CURL handle");
        return false;
    }

    MemoryBuffer body;
    memory_buffer_init(&body);
    HeaderAccumulator headers;
    header_accumulator_init(&headers);
    struct curl_slist *request_headers = NULL;
    char curl_error[CURL_ERROR_SIZE];
    curl_error[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_body_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "able-http-client/0.1");

    if (strcasecmp(method, "GET") == 0)
    {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }
    else if (strcasecmp(method, "POST") == 0)
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    }
    else
    {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    }

    if (options)
    {
        if (options->body)
        {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, options->body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(options->body));
        }

        if (options->headers && options->header_count > 0)
        {
            for (size_t i = 0; i < options->header_count; ++i)
            {
                const char *name = options->headers[i].name ? options->headers[i].name : "";
                const char *value = options->headers[i].value ? options->headers[i].value : "";
                if (!try_append_header(&request_headers, name, value))
                {
                    if (error_message)
                        *error_message = strdup("Failed to allocate header");
                    goto cleanup_failure;
                }
            }
        }

        if (options->cache_control && !try_append_header(&request_headers, "Cache-Control", options->cache_control))
        {
            if (error_message)
                *error_message = strdup("Failed to allocate header");
            goto cleanup_failure;
        }
        if (options->integrity && !try_append_header(&request_headers, "Integrity", options->integrity))
        {
            if (error_message)
                *error_message = strdup("Failed to allocate header");
            goto cleanup_failure;
        }
        if (options->credentials)
        {
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
            curl_easy_setopt(curl, CURLOPT_USERPWD, options->credentials);
        }
        if (options->refferer)
            curl_easy_setopt(curl, CURLOPT_REFERER, options->refferer);
    }

    if (request_headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, request_headers);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        if (error_message)
        {
            if (curl_error[0] != '\0')
                *error_message = strdup(curl_error);
            else
            {
                const char *msg = curl_easy_strerror(res);
                *error_message = strdup(msg ? msg : "HTTP request failed");
            }
        }
        goto cleanup_failure;
    }

    if (request_headers)
        curl_slist_free_all(request_headers);

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    char *effective_url = NULL;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);

    response->status_code = status;
    response->body = body.data;
    body.data = NULL;

    if (headers.status_text)
    {
        response->status_text = headers.status_text;
        headers.status_text = NULL;
    }
    else
    {
        response->status_text = strdup("");
    }

    response->headers = headers.headers;
    response->header_count = headers.count;
    headers.headers = NULL;
    headers.count = 0;

    if (effective_url)
        response->final_url = strdup(effective_url);
    else
        response->final_url = strdup(url);

    header_accumulator_free(&headers);
    memory_buffer_free(&body);
    curl_easy_cleanup(curl);
    return true;

cleanup_failure:
    if (request_headers)
        curl_slist_free_all(request_headers);
    memory_buffer_free(&body);
    header_accumulator_free(&headers);
    curl_easy_cleanup(curl);
    return false;
}

void http_response_cleanup(HttpResponse *response)
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
        for (size_t i = 0; i < response->header_count; ++i)
        {
            free(response->headers[i].name);
            free(response->headers[i].value);
        }
        free(response->headers);
        response->headers = NULL;
    }
    response->header_count = 0;
}
