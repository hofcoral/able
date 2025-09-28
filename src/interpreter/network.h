#ifndef INTERPRETER_NETWORK_H
#define INTERPRETER_NETWORK_H

#include <stdbool.h>

#include "types/value.h"

bool network_is_http_method(const char *name);
Value network_execute(const char *method,
                      const Value *args,
                      int arg_count,
                      int line,
                      int column);

#endif
