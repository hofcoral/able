
#include <stdio.h> // for malloc, free, exit
#include <stdlib.h> // for malloc, free, exit
#include <string.h> // for strdup, strcmp

#include "types/variable.h"
#include "types/value.h"
#include "types/object.h"
#include "utils/utils.h"

void set_variable(const char *name, Value val)
{
    for (int i = 0; i < var_count; i++)
    {
        if (strcmp(vars[i].name, name) == 0)
        {
            free_value(vars[i].value); // Free old value

            // Assign deep-copied value
            vars[i].value.type = val.type;
            switch (val.type)
            {
            case VAL_STRING:
                vars[i].value.str = strdup(val.str);
                break;
            case VAL_NUMBER:
                vars[i].value.num = val.num;
                break;
            case VAL_OBJECT:
                vars[i].value.obj = clone_object(val.obj);
                break;
            case VAL_FUNCTION:
                vars[i].value.func = val.func;
                break;
            default:
                break;
            }
            return;
        }
    }

    // New variable
    vars[var_count].name = strdup(name);
    vars[var_count].value.type = val.type;
    switch (val.type)
    {
    case VAL_STRING:
        vars[var_count].value.str = strdup(val.str);
        break;
    case VAL_NUMBER:
        vars[var_count].value.num = val.num;
        break;
    case VAL_OBJECT:
        vars[var_count].value.obj = clone_object(val.obj);
        break;
    case VAL_FUNCTION:
        vars[var_count].value.func = val.func;
        break;
    default:
        break;
    }

    var_count++;
}

Value get_variable(const char *name)
{
    for (int i = 0; i < var_count; i++)
    {
        if (strcmp(vars[i].name, name) == 0)
        {
            return vars[i].value; // safe if caller doesn't free
        }
    }
    log_error("Runtime error: variable '%s' is not defined.", name);
    exit(1);
}

void free_vars()
{
    for (int i = 0; i < var_count; i++)
    {
        free(vars[i].name);
        free_value(vars[i].value);
    }
    var_count = 0;
}
