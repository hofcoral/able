#include "utils/json.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/list.h"
#include "types/object.h"
#include "types/value.h"

typedef struct
{
    char *data;
    size_t length;
    size_t capacity;
} JsonBuffer;

static void buffer_init(JsonBuffer *buffer)
{
    buffer->data = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
}

static void buffer_free(JsonBuffer *buffer)
{
    free(buffer->data);
    buffer->data = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
}

static bool buffer_reserve(JsonBuffer *buffer, size_t additional)
{
    size_t required = buffer->length + additional + 1;
    if (required <= buffer->capacity)
        return true;

    size_t new_capacity = buffer->capacity ? buffer->capacity : 64;
    while (new_capacity < required)
    {
        if (new_capacity > SIZE_MAX / 2)
        {
            new_capacity = required;
            break;
        }
        new_capacity *= 2;
    }

    char *resized = realloc(buffer->data, new_capacity);
    if (!resized)
        return false;

    buffer->data = resized;
    buffer->capacity = new_capacity;
    return true;
}

static bool buffer_append(JsonBuffer *buffer, const char *data, size_t length)
{
    if (!buffer_reserve(buffer, length))
        return false;
    memcpy(buffer->data + buffer->length, data, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
    return true;
}

static bool buffer_append_char(JsonBuffer *buffer, char c)
{
    return buffer_append(buffer, &c, 1);
}

static bool buffer_append_str(JsonBuffer *buffer, const char *str)
{
    return buffer_append(buffer, str, strlen(str));
}

static bool set_error(char **error, const char *fmt, ...)
{
    if (!error)
        return false;

    va_list args;
    va_start(args, fmt);
    char tmp[256];
    vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end(args);

    char *msg = strdup(tmp);
    if (!msg)
    {
        *error = NULL;
        return false;
    }
    *error = msg;
    return false;
}

static bool append_escaped_string(JsonBuffer *buffer, const char *str)
{
    if (!buffer_append_char(buffer, '"'))
        return false;

    for (const unsigned char *p = (const unsigned char *)str; *p; ++p)
    {
        switch (*p)
        {
        case '"':
            if (!buffer_append_str(buffer, "\\\""))
                return false;
            break;
        case '\\':
            if (!buffer_append_str(buffer, "\\\\"))
                return false;
            break;
        case '\b':
            if (!buffer_append_str(buffer, "\\b"))
                return false;
            break;
        case '\f':
            if (!buffer_append_str(buffer, "\\f"))
                return false;
            break;
        case '\n':
            if (!buffer_append_str(buffer, "\\n"))
                return false;
            break;
        case '\r':
            if (!buffer_append_str(buffer, "\\r"))
                return false;
            break;
        case '\t':
            if (!buffer_append_str(buffer, "\\t"))
                return false;
            break;
        default:
            if (*p < 0x20)
            {
                char buf[7];
                snprintf(buf, sizeof(buf), "\\u%04x", *p);
                if (!buffer_append_str(buffer, buf))
                    return false;
            }
            else
            {
                if (!buffer_append_char(buffer, (char)*p))
                    return false;
            }
            break;
        }
    }

    if (!buffer_append_char(buffer, '"'))
        return false;

    return true;
}

static bool stringify_value(JsonBuffer *buffer, const Value *value, char **error)
{
    if (!value)
        return buffer_append_str(buffer, "null");

    switch (value->type)
    {
    case VAL_UNDEFINED:
    case VAL_NULL:
        return buffer_append_str(buffer, "null");
    case VAL_BOOL:
        return buffer_append_str(buffer, value->boolean ? "true" : "false");
    case VAL_NUMBER:
    {
        char numbuf[64];
        snprintf(numbuf, sizeof(numbuf), "%.15g", value->num);
        return buffer_append_str(buffer, numbuf);
    }
    case VAL_STRING:
        return append_escaped_string(buffer, value->str ? value->str : "");
    case VAL_LIST:
    {
        if (!buffer_append_char(buffer, '['))
            return false;
        if (value->list)
        {
            for (int i = 0; i < value->list->count; ++i)
            {
                if (i > 0 && !buffer_append_char(buffer, ','))
                    return false;
                if (!stringify_value(buffer, &value->list->items[i], error))
                    return false;
            }
        }
        return buffer_append_char(buffer, ']');
    }
    case VAL_OBJECT:
    {
        if (!buffer_append_char(buffer, '{'))
            return false;
        if (value->obj)
        {
            for (int i = 0; i < value->obj->count; ++i)
            {
                if (i > 0 && !buffer_append_char(buffer, ','))
                    return false;
                if (!append_escaped_string(buffer, value->obj->pairs[i].key))
                    return false;
                if (!buffer_append_char(buffer, ':'))
                    return false;
                if (!stringify_value(buffer, &value->obj->pairs[i].value, error))
                    return false;
            }
        }
        return buffer_append_char(buffer, '}');
    }
    default:
        return set_error(error, "Unsupported type for JSON serialization");
    }
}

bool json_stringify_value(const Value *value, char **out_json, char **error_message)
{
    if (!out_json)
        return set_error(error_message, "Output pointer is NULL");

    if (error_message)
        *error_message = NULL;

    JsonBuffer buffer;
    buffer_init(&buffer);

    if (!stringify_value(&buffer, value, error_message))
    {
        buffer_free(&buffer);
        if (error_message && !*error_message)
            set_error(error_message, "Failed to serialize value to JSON");
        return false;
    }

    *out_json = buffer.data ? buffer.data : strdup("null");
    if (!*out_json)
    {
        buffer_free(&buffer);
        return set_error(error_message, "Out of memory");
    }
    return true;
}

static bool buffer_append_codepoint(JsonBuffer *buffer, unsigned int codepoint)
{
    if (codepoint <= 0x7F)
    {
        return buffer_append_char(buffer, (char)codepoint);
    }
    else if (codepoint <= 0x7FF)
    {
        char seq[2];
        seq[0] = (char)(0xC0 | ((codepoint >> 6) & 0x1F));
        seq[1] = (char)(0x80 | (codepoint & 0x3F));
        return buffer_append(buffer, seq, 2);
    }
    else if (codepoint <= 0xFFFF)
    {
        char seq[3];
        seq[0] = (char)(0xE0 | ((codepoint >> 12) & 0x0F));
        seq[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        seq[2] = (char)(0x80 | (codepoint & 0x3F));
        return buffer_append(buffer, seq, 3);
    }
    else if (codepoint <= 0x10FFFF)
    {
        char seq[4];
        seq[0] = (char)(0xF0 | ((codepoint >> 18) & 0x07));
        seq[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        seq[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        seq[3] = (char)(0x80 | (codepoint & 0x3F));
        return buffer_append(buffer, seq, 4);
    }
    return false;
}

static int hex_value(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

typedef struct
{
    const char *text;
    size_t index;
} JsonParser;

static void parser_init(JsonParser *parser, const char *json)
{
    parser->text = json ? json : "";
    parser->index = 0;
}

static char parser_peek(const JsonParser *parser)
{
    return parser->text[parser->index];
}

static char parser_next(JsonParser *parser)
{
    return parser->text[parser->index++];
}

static void parser_skip_whitespace(JsonParser *parser)
{
    while (isspace((unsigned char)parser->text[parser->index]))
        parser->index++;
}

static bool parser_consume(JsonParser *parser, char expected)
{
    if (parser->text[parser->index] == expected)
    {
        parser->index++;
        return true;
    }
    return false;
}

static bool parse_value(JsonParser *parser, Value *out, char **error);

static bool parse_string(JsonParser *parser, Value *out, char **error)
{
    if (!parser_consume(parser, '"'))
        return set_error(error, "Expected '\"' at position %zu", parser->index);

    JsonBuffer buffer;
    buffer_init(&buffer);

    while (true)
    {
        char c = parser_next(parser);
        if (c == '\0')
        {
            buffer_free(&buffer);
            return set_error(error, "Unterminated string at position %zu", parser->index);
        }
        if (c == '"')
            break;
        if (c == '\\')
        {
            char esc = parser_next(parser);
            if (esc == '\0')
            {
                buffer_free(&buffer);
                return set_error(error, "Unterminated escape sequence at position %zu", parser->index);
            }
            switch (esc)
            {
            case '"':
            case '\\':
            case '/':
                if (!buffer_append_char(&buffer, esc))
                {
                    buffer_free(&buffer);
                    return set_error(error, "Out of memory");
                }
                break;
            case 'b':
                if (!buffer_append_char(&buffer, '\b'))
                {
                    buffer_free(&buffer);
                    return set_error(error, "Out of memory");
                }
                break;
            case 'f':
                if (!buffer_append_char(&buffer, '\f'))
                {
                    buffer_free(&buffer);
                    return set_error(error, "Out of memory");
                }
                break;
            case 'n':
                if (!buffer_append_char(&buffer, '\n'))
                {
                    buffer_free(&buffer);
                    return set_error(error, "Out of memory");
                }
                break;
            case 'r':
                if (!buffer_append_char(&buffer, '\r'))
                {
                    buffer_free(&buffer);
                    return set_error(error, "Out of memory");
                }
                break;
            case 't':
                if (!buffer_append_char(&buffer, '\t'))
                {
                    buffer_free(&buffer);
                    return set_error(error, "Out of memory");
                }
                break;
            case 'u':
            {
                unsigned int codepoint = 0;
                for (int i = 0; i < 4; ++i)
                {
                    char hex = parser_next(parser);
                    int value = hex_value(hex);
                    if (value < 0)
                    {
                        buffer_free(&buffer);
                        return set_error(error, "Invalid unicode escape at position %zu", parser->index);
                    }
                    codepoint = (codepoint << 4) | (unsigned int)value;
                }

                if (codepoint >= 0xD800 && codepoint <= 0xDBFF)
                {
                    char lead1 = parser_next(parser);
                    char lead2 = parser_next(parser);
                    if (lead1 != '\\' || lead2 != 'u')
                    {
                        buffer_free(&buffer);
                        return set_error(error, "Invalid unicode surrogate pair at position %zu", parser->index);
                    }

                    unsigned int low = 0;
                    for (int i = 0; i < 4; ++i)
                    {
                        char hex = parser_next(parser);
                        int value = hex_value(hex);
                        if (value < 0)
                        {
                            buffer_free(&buffer);
                            return set_error(error, "Invalid unicode surrogate pair at position %zu", parser->index);
                        }
                        low = (low << 4) | (unsigned int)value;
                    }
                    if (low < 0xDC00 || low > 0xDFFF)
                    {
                        buffer_free(&buffer);
                        return set_error(error, "Invalid unicode surrogate pair at position %zu", parser->index);
                    }
                    codepoint = 0x10000 + (((codepoint - 0xD800) << 10) | (low - 0xDC00));
                }

                if (!buffer_append_codepoint(&buffer, codepoint))
                {
                    buffer_free(&buffer);
                    return set_error(error, "Out of memory");
                }
                break;
            }
            default:
                buffer_free(&buffer);
                return set_error(error, "Invalid escape sequence at position %zu", parser->index);
            }
        }
        else
        {
            if (!buffer_append_char(&buffer, c))
            {
                buffer_free(&buffer);
                return set_error(error, "Out of memory");
            }
        }
    }

    out->type = VAL_STRING;
    out->str = buffer.data ? buffer.data : strdup("");
    if (!out->str)
    {
        buffer_free(&buffer);
        return set_error(error, "Out of memory");
    }
    return true;
}

static bool parse_literal(JsonParser *parser, const char *literal)
{
    size_t start = parser->index;
    for (size_t i = 0; literal[i]; ++i)
    {
        if (parser->text[start + i] != literal[i])
            return false;
    }
    parser->index += strlen(literal);
    return true;
}

static bool parse_number(JsonParser *parser, Value *out, char **error)
{
    const char *start = parser->text + parser->index;
    char *endptr = NULL;
    double value = strtod(start, &endptr);
    if (endptr == start)
        return set_error(error, "Invalid number at position %zu", parser->index);

    parser->index = (size_t)(endptr - parser->text);

    char next = parser_peek(parser);
    if (!(next == '\0' || next == ',' || next == '}' || next == ']' || isspace((unsigned char)next)))
        return set_error(error, "Invalid character after number at position %zu", parser->index);

    out->type = VAL_NUMBER;
    out->num = value;
    return true;
}

static bool parse_array(JsonParser *parser, Value *out, char **error)
{
    if (!parser_consume(parser, '['))
        return set_error(error, "Expected '[' at position %zu", parser->index);

    List *list = malloc(sizeof(List));
    if (!list)
        return set_error(error, "Out of memory");
    list->count = 0;
    list->capacity = 0;
    list->items = NULL;

    parser_skip_whitespace(parser);
    if (parser_consume(parser, ']'))
    {
        out->type = VAL_LIST;
        out->list = list;
        return true;
    }

    while (true)
    {
        Value item = {.type = VAL_NULL};
        if (!parse_value(parser, &item, error))
        {
            free_list(list);
            return false;
        }
        list_append(list, item);
        free_value(item);

        parser_skip_whitespace(parser);
        if (parser_consume(parser, ']'))
            break;
        if (!parser_consume(parser, ','))
        {
            free_list(list);
            return set_error(error, "Expected ',' or ']' at position %zu", parser->index);
        }
        parser_skip_whitespace(parser);
    }

    out->type = VAL_LIST;
    out->list = list;
    return true;
}

static bool parse_object(JsonParser *parser, Value *out, char **error)
{
    if (!parser_consume(parser, '{'))
        return set_error(error, "Expected '{' at position %zu", parser->index);

    Object *obj = object_create();
    if (!obj)
        return set_error(error, "Out of memory");

    parser_skip_whitespace(parser);
    if (parser_consume(parser, '}'))
    {
        out->type = VAL_OBJECT;
        out->obj = obj;
        return true;
    }

    while (true)
    {
        Value key_val = {.type = VAL_NULL};
        if (!parse_string(parser, &key_val, error))
        {
            free_object(obj);
            return false;
        }

        parser_skip_whitespace(parser);
        if (!parser_consume(parser, ':'))
        {
            free(key_val.str);
            free_object(obj);
            return set_error(error, "Expected ':' at position %zu", parser->index);
        }

        parser_skip_whitespace(parser);
        Value val = {.type = VAL_NULL};
        if (!parse_value(parser, &val, error))
        {
            free(key_val.str);
            free_object(obj);
            return false;
        }

        object_set(obj, key_val.str, val);
        free(key_val.str);
        free_value(val);

        parser_skip_whitespace(parser);
        if (parser_consume(parser, '}'))
            break;
        if (!parser_consume(parser, ','))
        {
            free_object(obj);
            return set_error(error, "Expected ',' or '}' at position %zu", parser->index);
        }
        parser_skip_whitespace(parser);
    }

    out->type = VAL_OBJECT;
    out->obj = obj;
    return true;
}

static bool parse_value(JsonParser *parser, Value *out, char **error)
{
    parser_skip_whitespace(parser);
    char c = parser_peek(parser);

    if (c == '"')
        return parse_string(parser, out, error);
    if (c == '-' || (c >= '0' && c <= '9'))
        return parse_number(parser, out, error);
    if (c == 't')
    {
        if (!parse_literal(parser, "true"))
            return set_error(error, "Invalid literal at position %zu", parser->index);
        out->type = VAL_BOOL;
        out->boolean = true;
        return true;
    }
    if (c == 'f')
    {
        if (!parse_literal(parser, "false"))
            return set_error(error, "Invalid literal at position %zu", parser->index);
        out->type = VAL_BOOL;
        out->boolean = false;
        return true;
    }
    if (c == 'n')
    {
        if (!parse_literal(parser, "null"))
            return set_error(error, "Invalid literal at position %zu", parser->index);
        out->type = VAL_NULL;
        return true;
    }
    if (c == '[')
        return parse_array(parser, out, error);
    if (c == '{')
        return parse_object(parser, out, error);

    return set_error(error, "Unexpected character '%c' at position %zu", c, parser->index);
}

bool json_parse_string(const char *json, Value *out_value, char **error_message)
{
    if (!out_value)
        return set_error(error_message, "Output value pointer is NULL");

    if (error_message)
        *error_message = NULL;

    JsonParser parser;
    parser_init(&parser, json);

    if (!parse_value(&parser, out_value, error_message))
        return false;

    parser_skip_whitespace(&parser);
    if (parser_peek(&parser) != '\0')
    {
        free_value(*out_value);
        out_value->type = VAL_UNDEFINED;
        return set_error(error_message, "Unexpected trailing characters at position %zu", parser.index);
    }

    return true;
}
