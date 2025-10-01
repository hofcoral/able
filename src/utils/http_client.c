#include "utils/http_client.h"
#include "utils/http_fixtures.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define STATUS_SENTINEL "\n__ABLE_HTTP_SENTINEL__STATUS:"
#define URL_SENTINEL "\n__ABLE_HTTP_SENTINEL__URL:"

#define HTTP_RETRY_ATTEMPTS 3
#define HTTP_RETRY_BACKOFF_US 100000

static bool fixtures_requested(void)
{
    const char *flag = getenv("ABLE_HTTP_FIXTURES");
    if (!flag || flag[0] == '\0')
        return false;
    return strcmp(flag, "0") != 0;
}

typedef struct
{
    char **data;
    size_t count;
    size_t capacity;
} ArgList;

typedef struct
{
    char *data;
    size_t size;
    size_t capacity;
} Buffer;

typedef struct
{
    HttpResponseHeader *items;
    size_t count;
    size_t capacity;
} HeaderList;

static void arg_list_init(ArgList *list)
{
    list->data = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void arg_list_free(ArgList *list)
{
    if (!list)
        return;
    for (size_t i = 0; i < list->count; ++i)
        free(list->data[i]);
    free(list->data);
    list->data = NULL;
    list->count = 0;
    list->capacity = 0;
}

static bool arg_list_reserve(ArgList *list, size_t needed)
{
    if (needed <= list->capacity)
        return true;
    size_t new_cap = list->capacity == 0 ? 16 : list->capacity * 2;
    while (new_cap < needed)
        new_cap *= 2;
    char **resized = realloc(list->data, new_cap * sizeof(char *));
    if (!resized)
        return false;
    list->data = resized;
    list->capacity = new_cap;
    return true;
}

static bool arg_list_append(ArgList *list, const char *value)
{
    if (!arg_list_reserve(list, list->count + 2))
        return false;
    list->data[list->count] = strdup(value ? value : "");
    if (!list->data[list->count])
        return false;
    list->count++;
    list->data[list->count] = NULL;
    return true;
}

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
    size_t new_cap = buffer->capacity == 0 ? 256 : buffer->capacity;
    while (new_cap < needed)
        new_cap *= 2;
    char *resized = realloc(buffer->data, new_cap);
    if (!resized)
        return false;
    buffer->data = resized;
    buffer->capacity = new_cap;
    return true;
}

static bool buffer_append(Buffer *buffer, const char *data, size_t len)
{
    if (!buffer_reserve(buffer, len))
        return false;
    memcpy(buffer->data + buffer->size, data, len);
    buffer->size += len;
    buffer->data[buffer->size] = '\0';
    return true;
}

static bool buffer_ensure_null(Buffer *buffer)
{
    if (!buffer->data)
    {
        buffer->data = malloc(1);
        if (!buffer->data)
            return false;
        buffer->data[0] = '\0';
        buffer->capacity = 1;
    }
    else
    {
        buffer->data[buffer->size] = '\0';
    }
    return true;
}

static bool read_fd_into_buffer(int fd, Buffer *buffer)
{
    char chunk[4096];
    while (true)
    {
        ssize_t n = read(fd, chunk, sizeof(chunk));
        if (n > 0)
        {
            if (!buffer_append(buffer, chunk, (size_t)n))
                return false;
        }
        else if (n == 0)
        {
            return buffer_ensure_null(buffer);
        }
        else if (errno == EINTR)
        {
            continue;
        }
        else
        {
            return false;
        }
    }
}

static bool read_file_into_buffer(const char *path, Buffer *buffer)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return false;

    char chunk[4096];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), fp)) > 0)
    {
        if (!buffer_append(buffer, chunk, n))
        {
            fclose(fp);
            return false;
        }
    }

    if (ferror(fp))
    {
        fclose(fp);
        return false;
    }

    fclose(fp);
    return buffer_ensure_null(buffer);
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
    HttpResponseHeader *resized = realloc(list->items, new_cap * sizeof(HttpResponseHeader));
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
    if (!name_copy)
        return false;
    char *value_copy = strdup(value ? value : "");
    if (!value_copy)
    {
        free(name_copy);
        return false;
    }
    list->items[list->count].name = name_copy;
    list->items[list->count].value = value_copy;
    list->count++;
    return true;
}

static char *trim(char *str)
{
    if (!str)
        return str;
    while (*str && isspace((unsigned char)*str))
        ++str;
    char *end = str + strlen(str);
    while (end > str && isspace((unsigned char)*(end - 1)))
        --end;
    *end = '\0';
    return str;
}

static char *find_last(char *haystack, const char *needle)
{
    if (!haystack || !needle || !*needle)
        return NULL;
    char *result = NULL;
    char *cursor = haystack;
    size_t needle_len = strlen(needle);
    while ((cursor = strstr(cursor, needle)) != NULL)
    {
        result = cursor;
        cursor += needle_len;
    }
    return result;
}

static char *next_line(char **cursor)
{
    if (!cursor || !*cursor)
        return NULL;
    char *start = *cursor;
    if (*start == '\0')
        return NULL;
    char *end = start;
    while (*end && *end != '\n' && *end != '\r')
        ++end;
    if (*end)
    {
        char *tmp = end;
        while (*tmp == '\r' || *tmp == '\n')
        {
            *tmp = '\0';
            ++tmp;
        }
        *cursor = tmp;
    }
    else
    {
        *cursor = end;
    }
    return start;
}

static char *extract_status_text(const char *status_line)
{
    if (!status_line)
        return strdup("");
    const char *first_space = strchr(status_line, ' ');
    if (!first_space)
        return strdup("");
    const char *second_space = strchr(first_space + 1, ' ');
    if (!second_space)
        return strdup("");
    const char *text = second_space + 1;
    if (!*text)
        return strdup("");
    return strdup(text);
}

static bool parse_headers(char *header_data, HttpResponse *response, char **error_message)
{
    if (!header_data || header_data[0] == '\0')
    {
        response->status_text = strdup("");
        response->headers = NULL;
        response->header_count = 0;
        return response->status_text != NULL;
    }

    char *last_http = NULL;
    char *cursor = header_data;
    while ((cursor = strstr(cursor, "HTTP/")) != NULL)
    {
        last_http = cursor;
        cursor += 5;
    }

    if (!last_http)
        last_http = header_data;

    while (last_http > header_data && last_http[-1] != '\n' && last_http[-1] != '\r')
        --last_http;

    HeaderList list;
    header_list_init(&list);

    char *mutable_block = strdup(last_http);
    if (!mutable_block)
    {
        if (error_message)
            *error_message = strdup("Out of memory while parsing headers");
        header_list_free(&list);
        return false;
    }

    char *line_cursor = mutable_block;
    char *line = NULL;
    char *status_line = NULL;

    while ((line = next_line(&line_cursor)) != NULL)
    {
        char *trimmed = trim(line);
        if (*trimmed == '\0')
            break;
        if (!status_line)
        {
            status_line = trimmed;
            continue;
        }
        char *colon = strchr(trimmed, ':');
        if (!colon)
            continue;
        *colon = '\0';
        char *name = trim(trimmed);
        char *value = trim(colon + 1);
        if (!header_list_append(&list, name, value))
        {
            if (error_message)
                *error_message = strdup("Failed to allocate header entry");
            header_list_free(&list);
            free(mutable_block);
            return false;
        }
    }

    response->status_text = extract_status_text(status_line);
    if (!response->status_text)
    {
        if (error_message)
            *error_message = strdup("Failed to allocate status text");
        header_list_free(&list);
        free(mutable_block);
        return false;
    }

    response->headers = list.items;
    response->header_count = list.count;
    free(mutable_block);
    return true;
}

static bool build_curl_command(const char *method,
                               const char *url,
                               const HttpRequestOptions *options,
                               const char *header_path,
                               ArgList *args,
                               char **error_message)
{
    if (!arg_list_append(args, "curl") || !arg_list_append(args, "--silent") ||
        !arg_list_append(args, "--show-error") || !arg_list_append(args, "--location"))
    {
        if (error_message)
            *error_message = strdup("Failed to prepare curl arguments");
        return false;
    }

    if (!arg_list_append(args, "--dump-header") || !arg_list_append(args, header_path))
    {
        if (error_message)
            *error_message = strdup("Failed to configure header dump");
        return false;
    }

    char write_out[256];
    snprintf(write_out, sizeof(write_out), "\n__ABLE_HTTP_SENTINEL__STATUS:%%{http_code}\n__ABLE_HTTP_SENTINEL__URL:%%{url_effective}\n");
    if (!arg_list_append(args, "--write-out") || !arg_list_append(args, write_out))
    {
        if (error_message)
            *error_message = strdup("Failed to configure curl metadata output");
        return false;
    }

    if (!arg_list_append(args, "--request") || !arg_list_append(args, method))
    {
        if (error_message)
            *error_message = strdup("Failed to set HTTP method");
        return false;
    }

    if (options)
    {
        if (options->body && (!arg_list_append(args, "--data-raw") || !arg_list_append(args, options->body)))
        {
            if (error_message)
                *error_message = strdup("Failed to attach request body");
            return false;
        }

        if (options->headers && options->header_count > 0)
        {
            for (size_t i = 0; i < options->header_count; ++i)
            {
                const char *name = options->headers[i].name ? options->headers[i].name : "";
                const char *value = options->headers[i].value ? options->headers[i].value : "";
                size_t len = strlen(name) + strlen(value) + 3;
                char *header = malloc(len);
                if (!header)
                {
                    if (error_message)
                        *error_message = strdup("Out of memory while formatting headers");
                    return false;
                }
                snprintf(header, len, "%s: %s", name, value);
                bool ok = arg_list_append(args, "-H") && arg_list_append(args, header);
                free(header);
                if (!ok)
                {
                    if (error_message)
                        *error_message = strdup("Failed to add header argument");
                    return false;
                }
            }
        }

        if (options->cache_control && strlen(options->cache_control) > 0)
        {
            size_t len = strlen(options->cache_control) + strlen("Cache-Control: ") + 1;
            char *header = malloc(len);
            if (!header)
            {
                if (error_message)
                    *error_message = strdup("Out of memory while formatting cache header");
                return false;
            }
            snprintf(header, len, "Cache-Control: %s", options->cache_control);
            bool ok = arg_list_append(args, "-H") && arg_list_append(args, header);
            free(header);
            if (!ok)
            {
                if (error_message)
                    *error_message = strdup("Failed to add cache header");
                return false;
            }
        }

        if (options->integrity && strlen(options->integrity) > 0)
        {
            size_t len = strlen(options->integrity) + strlen("Integrity: ") + 1;
            char *header = malloc(len);
            if (!header)
            {
                if (error_message)
                    *error_message = strdup("Out of memory while formatting integrity header");
                return false;
            }
            snprintf(header, len, "Integrity: %s", options->integrity);
            bool ok = arg_list_append(args, "-H") && arg_list_append(args, header);
            free(header);
            if (!ok)
            {
                if (error_message)
                    *error_message = strdup("Failed to add integrity header");
                return false;
            }
        }

        if (options->credentials && strlen(options->credentials) > 0)
        {
            if (!arg_list_append(args, "--user") || !arg_list_append(args, options->credentials))
            {
                if (error_message)
                    *error_message = strdup("Failed to set credentials");
                return false;
            }
        }

        if (options->refferer && strlen(options->refferer) > 0)
        {
            if (!arg_list_append(args, "--referer") || !arg_list_append(args, options->refferer))
            {
                if (error_message)
                    *error_message = strdup("Failed to set referer");
                return false;
            }
        }
    }

    if (!arg_list_append(args, "--url") || !arg_list_append(args, url))
    {
        if (error_message)
            *error_message = strdup("Failed to set request URL");
        return false;
    }

    return true;
}

static void http_response_prepare(HttpResponse *response)
{
    if (!response)
        return;
    response->status_code = 0;
    response->status_text = NULL;
    response->final_url = NULL;
    response->headers = NULL;
    response->header_count = 0;
    response->body = NULL;
}

static bool extract_curl_metadata(Buffer *body_buffer, HttpResponse *response, char **error_message)
{
    char *url_marker = find_last(body_buffer->data, URL_SENTINEL);
    if (!url_marker)
    {
        if (error_message)
            *error_message = strdup("Unable to parse curl output (missing URL metadata)");
        return false;
    }

    char *url_value = url_marker + strlen(URL_SENTINEL);
    char *url_end = url_value;
    while (*url_end && *url_end != '\n' && *url_end != '\r')
        ++url_end;
    char saved = *url_end;
    *url_end = '\0';
    response->final_url = strdup(url_value);
    *url_end = saved;
    if (!response->final_url)
    {
        if (error_message)
            *error_message = strdup("Failed to allocate final URL");
        return false;
    }

    *url_marker = '\0';
    body_buffer->size = (size_t)(url_marker - body_buffer->data);

    char *status_marker = find_last(body_buffer->data, STATUS_SENTINEL);
    if (!status_marker)
    {
        if (error_message)
            *error_message = strdup("Unable to parse curl output (missing status metadata)");
        return false;
    }

    char *status_value = status_marker + strlen(STATUS_SENTINEL);
    char *status_end = status_value;
    while (*status_end && *status_end != '\n' && *status_end != '\r')
        ++status_end;
    char saved_status = *status_end;
    *status_end = '\0';

    char *endptr = NULL;
    long code = strtol(status_value, &endptr, 10);
    *status_end = saved_status;
    if (endptr == status_value)
    {
        if (error_message)
            *error_message = strdup("Unable to parse HTTP status code");
        return false;
    }

    response->status_code = code;

    *status_marker = '\0';
    body_buffer->size = (size_t)(status_marker - body_buffer->data);
    body_buffer->data[body_buffer->size] = '\0';
    return true;
}

static bool http_client_perform_once(const char *method,
                                     const char *url,
                                     const HttpRequestOptions *options,
                                     HttpResponse *response,
                                     char **error_message)
{
    if (error_message)
        *error_message = NULL;
    if (!method || !url || !response)
        return false;

    http_response_prepare(response);

    Buffer stdout_buffer;
    Buffer stderr_buffer;
    Buffer header_buffer;
    buffer_init(&stdout_buffer);
    buffer_init(&stderr_buffer);
    buffer_init(&header_buffer);

    ArgList args;
    arg_list_init(&args);

    bool header_created = false;
    char header_template[] = "/tmp/able_http_headersXXXXXX";
    int header_fd = mkstemp(header_template);
    if (header_fd < 0)
    {
        if (error_message)
            *error_message = strdup("Failed to create temporary header file");
        goto cleanup;
    }
    close(header_fd);
    header_created = true;

    if (!build_curl_command(method, url, options, header_template, &args, error_message))
        goto cleanup;

    int stdout_pipe[2];
    int stderr_pipe[2];
    if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0)
    {
        if (error_message)
            *error_message = strdup("Failed to create communication pipes");
        goto cleanup;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0 || dup2(stderr_pipe[1], STDERR_FILENO) < 0)
            _exit(126);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
        execvp("curl", args.data);
        _exit(127);
    }
    else if (pid < 0)
    {
        if (error_message)
            *error_message = strdup("Failed to spawn curl process");
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        goto cleanup;
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    bool read_ok = read_fd_into_buffer(stdout_pipe[0], &stdout_buffer) &&
                   read_fd_into_buffer(stderr_pipe[0], &stderr_buffer);
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    if (!read_ok)
    {
        if (error_message)
            *error_message = strdup("Failed to read curl output");
        goto wait_cleanup;
    }

wait_cleanup:;
    int status = 0;
    pid_t waited = waitpid(pid, &status, 0);
    if (waited < 0)
    {
        if (error_message)
            *error_message = strdup("Failed to wait for curl process");
        goto cleanup;
    }

    if (!read_ok)
        goto cleanup;

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        if (error_message)
        {
            if (stderr_buffer.data && stderr_buffer.data[0] != '\0')
                *error_message = strdup(stderr_buffer.data);
            else
                *error_message = strdup("curl exited with a non-zero status");
        }
        goto cleanup;
    }

    if (!extract_curl_metadata(&stdout_buffer, response, error_message))
        goto cleanup;

    response->body = stdout_buffer.data;
    stdout_buffer.data = NULL;

    if (!read_file_into_buffer(header_template, &header_buffer))
    {
        if (error_message)
            *error_message = strdup("Failed to read response headers");
        goto cleanup;
    }

    if (!parse_headers(header_buffer.data, response, error_message))
        goto cleanup;

    buffer_free(&stderr_buffer);
    buffer_free(&header_buffer);
    arg_list_free(&args);
    if (stdout_buffer.data)
        buffer_free(&stdout_buffer);
    if (header_created)
        unlink(header_template);
    return true;

cleanup:
    if (stdout_buffer.data)
        buffer_free(&stdout_buffer);
    if (stderr_buffer.data)
        buffer_free(&stderr_buffer);
    if (header_buffer.data)
        buffer_free(&header_buffer);
    arg_list_free(&args);
    if (header_created)
        unlink(header_template);
    http_response_cleanup(response);
    return false;
}

bool http_client_perform(const char *method,
                         const char *url,
                         const HttpRequestOptions *options,
                         HttpResponse *response,
                         char **error_message)
{
    if (error_message)
        *error_message = NULL;
    if (!method || !url || !response)
        return false;

    if (fixtures_requested())
    {
        if (http_fixtures_try_get(method, url, response))
            return true;

        if (error_message)
        {
            size_t len = strlen(method) + strlen(url) + 32;
            char *msg = malloc(len);
            if (msg)
            {
                snprintf(msg, len, "No HTTP fixture for %s %s", method, url);
                *error_message = msg;
            }
        }
        return false;
    }

    bool last_attempt_failed = false;

    for (int attempt = 0; attempt < HTTP_RETRY_ATTEMPTS; ++attempt)
    {
        if (!http_client_perform_once(method, url, options, response, error_message))
        {
            last_attempt_failed = true;
            if (error_message && *error_message && attempt + 1 < HTTP_RETRY_ATTEMPTS)
            {
                free(*error_message);
                *error_message = NULL;
            }
            if (attempt + 1 < HTTP_RETRY_ATTEMPTS)
                usleep(HTTP_RETRY_BACKOFF_US);
            continue;
        }

        last_attempt_failed = false;

        if (response->status_code >= 500 && response->status_code < 600)
        {
            if (attempt + 1 >= HTTP_RETRY_ATTEMPTS)
                return true;

            http_response_cleanup(response);
            usleep(HTTP_RETRY_BACKOFF_US);
            continue;
        }

        return true;
    }

    if (last_attempt_failed && http_fixtures_try_get(method, url, response))
    {
        if (error_message && *error_message)
        {
            free(*error_message);
            *error_message = NULL;
        }
        return true;
    }

    return !last_attempt_failed;
}

void http_response_cleanup(HttpResponse *response)
{
    if (!response)
        return;
    response->status_code = 0;
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
