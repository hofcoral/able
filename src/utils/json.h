#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <stdbool.h>

#include "types/value.h"

bool json_stringify_value(const Value *value, char **out_json, char **error_message);
bool json_parse_string(const char *json, Value *out_value, char **error_message);

#endif
