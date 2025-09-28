#include "interpreter/network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/object.h"
#include "utils/http_client.h"
#include "utils/utils.h"

typedef struct
{
    char *body;
    char *cache_control;
    char *credentials;
    char *integrity;
    char *refferer;
    HttpRequestHeader *headers;
    size_t header_count;
} ParsedOptions;

static void parsed_options_init(ParsedOptions *opts)
{
    opts->body = NULL;
    opts->cache_control = NULL;
    opts->credentials = NULL;
    opts->integrity = NULL;
    opts->refferer = NULL;
    opts->headers = NULL;
    opts->header_count = 0;
}

static void parsed_options_cleanup(ParsedOptions *opts)
{
    if (!opts)
        return;
    free(opts->body);
    free(opts->cache_control);
    free(opts->credentials);
    free(opts->integrity);
    free(opts->refferer);
    if (opts->headers)
    {
        for (size_t i = 0; i < opts->header_count; ++i)
        {
            free((char *)opts->headers[i].name);
            free((char *)opts->headers[i].value);
        }
        free(opts->headers);
    }
    opts->headers = NULL;
    opts->header_count = 0;
}

static int object_try_get(Object *obj, const char *key, Value *out)
{
    if (!obj)
        return 0;
    for (int i = 0; i < obj->count; ++i)
    {
        if (strcmp(obj->pairs[i].key, key) == 0)
        {
            if (out)
                *out = obj->pairs[i].value;
            return 1;
        }
    }
    return 0;
}

static char *duplicate_string(const char *src)
{
    if (!src)
        return strdup("");
    return strdup(src);
}

static char *value_to_string(const Value *value, const char *field, int line, int column)
{
    if (!value)
        return NULL;
    switch (value->type)
    {
    case VAL_STRING:
        return duplicate_string(value->str ? value->str : "");
    case VAL_NUMBER:
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.15g", value->num);
        return strdup(buf);
    }
    case VAL_BOOL:
        return strdup(value->boolean ? "true" : "false");
    default:
        log_script_error(line, column, "%s must be a string-compatible value", field);
        exit(1);
    }
}

static void parse_headers(Object *headers_obj, ParsedOptions *opts, int line, int column)
{
    if (!headers_obj || headers_obj->count == 0)
        return;
    opts->headers = calloc((size_t)headers_obj->count, sizeof(HttpRequestHeader));
    if (!opts->headers)
    {
        log_script_error(line, column, "Failed to allocate headers");
        exit(1);
    }
    opts->header_count = (size_t)headers_obj->count;
    for (int i = 0; i < headers_obj->count; ++i)
    {
        opts->headers[i].name = duplicate_string(headers_obj->pairs[i].key);
        opts->headers[i].value = value_to_string(&headers_obj->pairs[i].value, "options.headers", line, column);
    }
}

static void parse_options(Object *options_obj, ParsedOptions *opts, int line, int column)
{
    Value value;
    if (object_try_get(options_obj, "body", &value))
        opts->body = value_to_string(&value, "options.body", line, column);
    if (object_try_get(options_obj, "cache", &value))
        opts->cache_control = value_to_string(&value, "options.cache", line, column);
    if (object_try_get(options_obj, "credentials", &value))
        opts->credentials = value_to_string(&value, "options.credentials", line, column);
    if (object_try_get(options_obj, "integrity", &value))
        opts->integrity = value_to_string(&value, "options.integrity", line, column);
    if (object_try_get(options_obj, "refferer", &value))
        opts->refferer = value_to_string(&value, "options.refferer", line, column);
    if (object_try_get(options_obj, "headers", &value))
    {
        if (value.type != VAL_OBJECT)
        {
            log_script_error(line, column, "options.headers must be an object");
            exit(1);
        }
        parse_headers(value.obj, opts, line, column);
    }
}

static Object *create_object(void)
{
    Object *obj = malloc(sizeof(Object));
    if (!obj)
        return NULL;
    obj->count = 0;
    obj->capacity = 0;
    obj->pairs = NULL;
    return obj;
}

static Value build_response_value(const char *method, const HttpResponse *response)
{
    Object *root = create_object();
    if (!root)
    {
        log_script_error(0, 0, "Out of memory while creating response object");
        exit(1);
    }

    Value status_val = {.type = VAL_NUMBER, .num = (double)response->status_code};
    object_set(root, "status", status_val);

    Value ok_val = {.type = VAL_BOOL, .boolean = response->status_code >= 200 && response->status_code < 300};
    object_set(root, "ok", ok_val);

    Value status_text_val = {.type = VAL_STRING, .str = duplicate_string(response->status_text ? response->status_text : "")};
    object_set(root, "statusText", status_text_val);
    free_value(status_text_val);

    Value url_val = {.type = VAL_STRING, .str = duplicate_string(response->final_url)};
    object_set(root, "url", url_val);
    free_value(url_val);

    Value body_val = {.type = VAL_STRING, .str = duplicate_string(response->body ? response->body : "")};
    object_set(root, "body", body_val);
    free_value(body_val);

    Object *headers_obj = create_object();
    if (!headers_obj)
    {
        free_object(root);
        log_script_error(0, 0, "Out of memory while creating headers object");
        exit(1);
    }
    for (size_t i = 0; i < response->header_count; ++i)
    {
        const char *name = response->headers[i].name ? response->headers[i].name : "";
        const char *value = response->headers[i].value ? response->headers[i].value : "";
        Value header_val = {.type = VAL_STRING, .str = duplicate_string(value)};
        object_set(headers_obj, name, header_val);
        free_value(header_val);
    }
    Value headers_val = {.type = VAL_OBJECT, .obj = headers_obj};
    object_set(root, "headers", headers_val);
    free_value(headers_val);

    Value method_val = {.type = VAL_STRING, .str = duplicate_string(method)};
    object_set(root, "method", method_val);
    free_value(method_val);

    Value result = {.type = VAL_OBJECT, .obj = root};
    return result;
}

bool network_is_http_method(const char *name)
{
    if (!name)
        return false;
    const char *methods[] = {"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS"};
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    {
        if (strcmp(name, methods[i]) == 0)
            return true;
    }
    return false;
}

Value network_execute(const char *method,
                      const Value *args,
                      int arg_count,
                      int line,
                      int column)
{
    if (arg_count < 1 || arg_count > 2)
    {
        log_script_error(line, column, "%s expects one or two arguments", method);
        exit(1);
    }
    const Value *url_val = &args[0];
    if (url_val->type != VAL_STRING)
    {
        log_script_error(line, column, "%s expects the first argument to be a string URL", method);
        exit(1);
    }

    const Value *options_val = arg_count > 1 ? &args[1] : NULL;
    Object *options_obj = NULL;
    if (options_val && options_val->type != VAL_UNDEFINED && options_val->type != VAL_NULL)
    {
        if (options_val->type != VAL_OBJECT)
        {
            log_script_error(line, column, "%s options must be an object", method);
            exit(1);
        }
        options_obj = options_val->obj;
    }

    ParsedOptions parsed;
    parsed_options_init(&parsed);
    HttpRequestOptions request_opts;
    memset(&request_opts, 0, sizeof(request_opts));

    if (options_obj)
    {
        parse_options(options_obj, &parsed, line, column);
        request_opts.body = parsed.body;
        request_opts.cache_control = parsed.cache_control;
        request_opts.credentials = parsed.credentials;
        request_opts.integrity = parsed.integrity;
        request_opts.refferer = parsed.refferer;
        request_opts.headers = parsed.headers;
        request_opts.header_count = parsed.header_count;
    }

    HttpResponse response = {0};
    char *error_message = NULL;
    bool ok = http_client_perform(method, url_val->str, options_obj ? &request_opts : NULL, &response, &error_message);

    parsed_options_cleanup(&parsed);

    if (!ok)
    {
        if (error_message)
        {
            log_script_error(line, column, "%s request failed: %s", method, error_message);
            free(error_message);
        }
        else
        {
            log_script_error(line, column, "%s request failed", method);
        }
        http_response_cleanup(&response);
        exit(1);
    }

    Value result = build_response_value(method, &response);
    http_response_cleanup(&response);
    return result;
}
