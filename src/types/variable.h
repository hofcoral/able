#ifndef VARIABLE_H
#define VARIABLE_H

#include "types/value.h"

#define MAX_VARS 128

typedef struct
{
    char *name;
    Value value;
} Variable;

static Variable vars[MAX_VARS];
static int var_count = 0;

void set_variable(const char *name, Value val);
Value get_variable(const char *name);

void free_vars();

#endif
