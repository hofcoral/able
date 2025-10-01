#include "interpreter/server.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "interpreter/interpreter.h"
#include "types/list.h"
#include "types/object.h"
#include "types/value.h"
#include "utils/http_server.h"
#include "utils/utils.h"

typedef struct
{
    char *method;
    char *path;
    Value handler;
} ServerRoute;

typedef struct
{
    ServerRoute *routes;
    size_t route_count;
    int call_line;
    int call_column;
} ServerContext;

static void server_route_cleanup(ServerRoute *route)
{
    if (!route)
        return;
    free(route->method);
    free(route->path);
    free_value(route->handler);
    route->method = NULL;
    route->path = NULL;
    route->handler.type = VAL_UNDEFINED;
}

static void server_context_cleanup(ServerContext *ctx)
{
    if (!ctx)
        return;
    for (size_t i = 0; i < ctx->route_count; ++i)
        server_route_cleanup(&ctx->routes[i]);
    free(ctx->routes);
    ctx->routes = NULL;
    ctx->route_count = 0;
}

static void fatal_script_error(int line, int column, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[ERROR in line %d:%d] ", line, column);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

static char *duplicate_string_checked(const char *src, int line, int column, const char *field)
{
    char *copy = strdup(src ? src : "");
    if (!copy)
        fatal_script_error(line, column, "Out of memory while copying %s", field);
    return copy;
}

static char *value_to_owned_string(const Value *value, int line, int column, const char *field)
{
    if (!value)
        return NULL;
    switch (value->type)
    {
    case VAL_STRING:
        return duplicate_string_checked(value->str, line, column, field);
    case VAL_NUMBER:
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.15g", value->num);
        return duplicate_string_checked(buf, line, column, field);
    }
    case VAL_BOOL:
        return duplicate_string_checked(value->boolean ? "true" : "false", line, column, field);
    default:
        fatal_script_error(line, column, "%s must be string-compatible", field);
    }

    return NULL;
}

static void ensure_route_handler_type(const Value *handler, int line, int column)
{
    if (!handler)
        fatal_script_error(line, column, "Route handler is missing");
    if (handler->type == VAL_FUNCTION || handler->type == VAL_BOUND_METHOD)
        return;
    fatal_script_error(line, column, "Route handler must be a function or bound method");
}

static void uppercase_inplace(char *text)
{
    if (!text)
        return;
    for (char *c = text; *c; ++c)
        *c = (char)toupper((unsigned char)*c);
}

static void parse_route(Object *route_obj, ServerRoute *route, int line, int column)
{
    Value *method_val = NULL;
    Value *path_val = NULL;
    Value *handler_val = NULL;

    for (int i = 0; i < route_obj->count; ++i)
    {
        if (strcmp(route_obj->pairs[i].key, "method") == 0)
            method_val = &route_obj->pairs[i].value;
        else if (strcmp(route_obj->pairs[i].key, "path") == 0)
            path_val = &route_obj->pairs[i].value;
        else if (strcmp(route_obj->pairs[i].key, "handler") == 0)
            handler_val = &route_obj->pairs[i].value;
    }

    if (!method_val || method_val->type != VAL_STRING)
        fatal_script_error(line, column, "Route requires a string method");
    if (!path_val || path_val->type != VAL_STRING)
        fatal_script_error(line, column, "Route requires a string path");
    ensure_route_handler_type(handler_val, line, column);

    route->method = duplicate_string_checked(method_val->str, line, column, "route.method");
    uppercase_inplace(route->method);
    route->path = duplicate_string_checked(path_val->str, line, column, "route.path");
    route->handler = clone_value(handler_val);
}

static void parse_routes(const Value *routes_value, ServerContext *ctx, int line, int column)
{
    if (!routes_value || routes_value->type != VAL_LIST)
        fatal_script_error(line, column, "server_listen config.routes must be a list");
    List *list = routes_value->list;
    if (!list || list->count == 0)
        fatal_script_error(line, column, "server_listen requires at least one route");

    ctx->routes = calloc((size_t)list->count, sizeof(ServerRoute));
    if (!ctx->routes)
        fatal_script_error(line, column, "Out of memory while preparing routes");
    ctx->route_count = (size_t)list->count;

    for (int i = 0; i < list->count; ++i)
    {
        Value entry = list->items[i];
        if (entry.type != VAL_OBJECT)
            fatal_script_error(line, column, "Each route must be an object");
        parse_route(entry.obj, &ctx->routes[i], line, column);
    }
}

static const ServerRoute *find_route(const ServerContext *ctx, const HttpServerRequest *request)
{
    for (size_t i = 0; i < ctx->route_count; ++i)
    {
        const ServerRoute *route = &ctx->routes[i];
        if (strcmp(route->method, request->method) == 0 && strcmp(route->path, request->path) == 0)
            return route;
    }
    return NULL;
}

static Object *create_object_checked(int line, int column, const char *context)
{
    Object *obj = object_create();
    if (!obj)
        fatal_script_error(line, column, "Out of memory while creating %s", context);
    return obj;
}

static Value build_request_value(const HttpServerRequest *request, const ServerContext *ctx)
{
    Object *root = create_object_checked(ctx->call_line, ctx->call_column, "request object");

    Value method_val = {.type = VAL_STRING, .str = duplicate_string_checked(request->method, ctx->call_line, ctx->call_column, "request.method")};
    object_set(root, "method", method_val);
    free_value(method_val);

    Value path_val = {.type = VAL_STRING, .str = duplicate_string_checked(request->path, ctx->call_line, ctx->call_column, "request.path")};
    object_set(root, "path", path_val);
    free_value(path_val);

    Value query_val = {.type = VAL_STRING, .str = duplicate_string_checked(request->query ? request->query : "", ctx->call_line, ctx->call_column, "request.query")};
    object_set(root, "query", query_val);
    free_value(query_val);

    Value version_val = {.type = VAL_STRING, .str = duplicate_string_checked(request->http_version, ctx->call_line, ctx->call_column, "request.httpVersion")};
    object_set(root, "httpVersion", version_val);
    free_value(version_val);

    Object *headers_obj = create_object_checked(ctx->call_line, ctx->call_column, "request headers");
    for (size_t i = 0; i < request->header_count; ++i)
    {
        Value header_val = {.type = VAL_STRING, .str = duplicate_string_checked(request->headers[i].value, ctx->call_line, ctx->call_column, "request header")};
        object_set(headers_obj, request->headers[i].name, header_val);
        free_value(header_val);
    }
    Value headers_val = {.type = VAL_OBJECT, .obj = headers_obj};
    object_set(root, "headers", headers_val);
    free_value(headers_val);

    Value body_val = {.type = VAL_STRING, .str = duplicate_string_checked(request->body ? request->body : "", ctx->call_line, ctx->call_column, "request.body")};
    object_set(root, "body", body_val);
    free_value(body_val);

    Value result = {.type = VAL_OBJECT, .obj = root};
    return result;
}

static bool apply_header_object(Object *headers_obj, HttpServerResponse *response, int line, int column, bool *has_content_type)
{
    for (int i = 0; i < headers_obj->count; ++i)
    {
        char *value = value_to_owned_string(&headers_obj->pairs[i].value, line, column, "response.headers value");
        if (!http_server_response_add_header(response, headers_obj->pairs[i].key, value))
        {
            free(value);
            return false;
        }
        if (strcasecmp(headers_obj->pairs[i].key, "Content-Type") == 0)
            *has_content_type = true;
        free(value);
    }
    return true;
}

static bool apply_response_object(const Value *result, HttpServerResponse *response, const ServerContext *ctx)
{
    Object *obj = result->obj;
    Value *status_val = NULL;
    Value *status_text_val = NULL;
    Value *body_val = NULL;
    Value *headers_val = NULL;

    for (int i = 0; i < obj->count; ++i)
    {
        if (strcmp(obj->pairs[i].key, "status") == 0)
            status_val = &obj->pairs[i].value;
        else if (strcmp(obj->pairs[i].key, "statusText") == 0)
            status_text_val = &obj->pairs[i].value;
        else if (strcmp(obj->pairs[i].key, "body") == 0)
            body_val = &obj->pairs[i].value;
        else if (strcmp(obj->pairs[i].key, "headers") == 0)
            headers_val = &obj->pairs[i].value;
    }

    if (status_val)
    {
        if (status_val->type != VAL_NUMBER)
            fatal_script_error(ctx->call_line, ctx->call_column, "response.status must be a number");
        int status_code = (int)status_val->num;
        if (!http_server_response_set_status(response, status_code, NULL))
            return false;
    }

    if (status_text_val)
    {
        if (status_text_val->type != VAL_STRING)
            fatal_script_error(ctx->call_line, ctx->call_column, "response.statusText must be a string");
        if (!http_server_response_set_status(response, response->status_code, status_text_val->str))
            return false;
    }

    bool has_content_type = false;
    if (headers_val)
    {
        if (headers_val->type != VAL_OBJECT)
            fatal_script_error(ctx->call_line, ctx->call_column, "response.headers must be an object");
        if (!apply_header_object(headers_val->obj, response, ctx->call_line, ctx->call_column, &has_content_type))
            return false;
    }

    if (body_val)
    {
        char *body = value_to_owned_string(body_val, ctx->call_line, ctx->call_column, "response.body");
        size_t length = strlen(body);
        if (!http_server_response_set_body(response, body, length))
        {
            free(body);
            return false;
        }
        if (!has_content_type)
        {
            if (!http_server_response_add_header(response, "Content-Type", "text/plain; charset=utf-8"))
            {
                free(body);
                return false;
            }
        }
        free(body);
    }

    return true;
}

static bool apply_response_value(const Value *result, HttpServerResponse *response, const ServerContext *ctx)
{
    if (!result)
        return true;
    if (result->type == VAL_UNDEFINED || result->type == VAL_NULL)
        return true;
    if (result->type == VAL_OBJECT)
        return apply_response_object(result, response, ctx);

    char *body = value_to_owned_string(result, ctx->call_line, ctx->call_column, "response");
    size_t length = strlen(body);
    bool ok = http_server_response_set_body(response, body, length);
    if (ok)
        ok = http_server_response_add_header(response, "Content-Type", "text/plain; charset=utf-8");
    free(body);
    return ok;
}

static bool server_handle_request(const HttpServerRequest *request, HttpServerResponse *response, void *user_data)
{
    ServerContext *ctx = (ServerContext *)user_data;
    const ServerRoute *route = find_route(ctx, request);
    if (!route)
    {
        http_server_response_set_status(response, 404, "Not Found");
        http_server_response_set_body(response, "Not Found", strlen("Not Found"));
        http_server_response_add_header(response, "Content-Type", "text/plain; charset=utf-8");
        return true;
    }

    Value request_value = build_request_value(request, ctx);
    Value result = interpreter_call_and_await(route->handler, &request_value, 1, ctx->call_line, ctx->call_column);
    free_value(request_value);

    if (!apply_response_value(&result, response, ctx))
    {
        http_server_response_set_status(response, 500, "Internal Server Error");
        http_server_response_set_body(response, "Internal Server Error", strlen("Internal Server Error"));
        http_server_response_add_header(response, "Content-Type", "text/plain; charset=utf-8");
    }

    free_value(result);
    return true;
}

static char *parse_port(const Value *value, int line, int column)
{
    if (!value)
        fatal_script_error(line, column, "server_listen requires a port");
    if (value->type == VAL_NUMBER)
    {
        if (value->num < 0 || value->num > 65535)
            fatal_script_error(line, column, "server_listen port must be between 0 and 65535");
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", (int)value->num);
        return duplicate_string_checked(buf, line, column, "port");
    }
    if (value->type == VAL_STRING)
        return duplicate_string_checked(value->str, line, column, "port");
    fatal_script_error(line, column, "server_listen port must be a string or number");
    return NULL;
}

static void parse_config(const Value *config,
                         char **host_out,
                         char **port_out,
                         ServerContext *ctx,
                         int line,
                         int column)
{
    if (!config || config->type != VAL_OBJECT)
        fatal_script_error(line, column, "server_listen expects a configuration object");

    Value *routes_value = NULL;
    Value *host_value = NULL;
    Value *port_value = NULL;

    Object *obj = config->obj;
    for (int i = 0; i < obj->count; ++i)
    {
        if (strcmp(obj->pairs[i].key, "routes") == 0)
            routes_value = &obj->pairs[i].value;
        else if (strcmp(obj->pairs[i].key, "host") == 0)
            host_value = &obj->pairs[i].value;
        else if (strcmp(obj->pairs[i].key, "port") == 0)
            port_value = &obj->pairs[i].value;
    }

    if (!routes_value)
        fatal_script_error(line, column, "server_listen requires routes");
    parse_routes(routes_value, ctx, line, column);

    if (host_value)
    {
        if (host_value->type != VAL_STRING)
            fatal_script_error(line, column, "server_listen host must be a string");
        *host_out = duplicate_string_checked(host_value->str, line, column, "host");
    }
    else
    {
        *host_out = duplicate_string_checked("0.0.0.0", line, column, "host");
    }

    *port_out = parse_port(port_value, line, column);
}

Value interpreter_server_listen(const Value *args, int arg_count, int line, int column)
{
    if (arg_count != 1)
        fatal_script_error(line, column, "server_listen expects exactly one argument");

    ServerContext ctx = {.routes = NULL, .route_count = 0, .call_line = line, .call_column = column};
    char *host = NULL;
    char *port = NULL;
    parse_config(&args[0], &host, &port, &ctx, line, column);

    char *error_message = NULL;
    bool ok = http_server_listen(host, port, server_handle_request, &ctx, &error_message);

    free(host);
    free(port);
    server_context_cleanup(&ctx);

    if (!ok)
    {
        if (error_message)
        {
            log_script_error(line, column, "server_listen failed: %s", error_message);
            free(error_message);
        }
        else
        {
            log_script_error(line, column, "server_listen failed");
        }
        exit(1);
    }

    Value undef = {.type = VAL_UNDEFINED};
    return undef;
}
